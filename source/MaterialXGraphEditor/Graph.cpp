//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include <MaterialXGraphEditor/Graph.h>

#include <MaterialXRenderGlsl/External/Glad/glad.h>

#include <GLFW/glfw3.h>

namespace
{

// the default node size is based off the of the size of the dot_color3 node using ed::getNodeSize() on that node
const ImVec2 DEFAULT_NODE_SIZE = ImVec2(138, 116);

const int DEFAULT_ALPHA = 255;
const int FILTER_ALPHA = 50;

// Function based off ImRect_Expanded function from ImGui Node Editor blueprints-example.cpp
ImRect expandImRect(const ImRect& rect, float x, float y)
{
    ImRect result = rect;
    result.Min.x -= x;
    result.Min.y -= y;
    result.Max.x += x;
    result.Max.y += y;
    return result;
}

} // anonymous namespace

Graph::Graph(const std::string& materialFilename,
             const std::string& meshFilename,
             const mx::FileSearchPath& searchPath,
             const mx::FilePathVec& libraryFolders) :
    _materialFilename(materialFilename),
    _searchPath(searchPath),
    _libraryFolders(libraryFolders),
    _initial(false),
    _delete(false),
    _fileDialogSave(ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_CreateNewDir),
    _isNodeGraph(false),
    _graphTotalSize(0),
    _popup(false),
    _shaderPopup(false),
    _searchNodeId(-1),
    _addNewNode(false),
    _ctrlClick(false),
    _isCut(false),
    _autoLayout(false),
    _frameCount(INT_MIN),
    _pinFilterType(mx::EMPTY_STRING)
{
    // Filter for MaterialX files for load and save
    mx::StringVec mtlxFilter;
    mtlxFilter.push_back(".mtlx");
    _fileDialog.SetTypeFilters(mtlxFilter);
    _fileDialogSave.SetTypeFilters(mtlxFilter);

    loadStandardLibraries();
    setPinColor();

    _graphDoc = loadDocument(materialFilename);
    _graphDoc->importLibrary(_stdLib);

    _initial = true;
    createNodeUIList(_stdLib);

    if (_graphDoc)
    {
        buildUiBaseGraph(_graphDoc);
        _currGraphElem = _graphDoc;
        _prevUiNode = nullptr;
    }

    // Create a renderer using the initial startup document.
    // Note that this document may have no nodes in it
    // if the material file name does not exist.
    mx::FilePath captureFilename = "resources/Materials/Examples/example.png";
    std::string envRadianceFilename = "resources/Lights/san_giuseppe_bridge_split.hdr";
    _renderer = std::make_shared<RenderView>(_graphDoc, meshFilename, envRadianceFilename,
                                             _searchPath, 256, 256);
    _renderer->initialize();
    _renderer->updateMaterials(nullptr);
    for (const std::string& incl : _renderer->getXincludeFiles())
    {
        _xincludeFiles.insert(incl);
    }
}

mx::ElementPredicate Graph::getElementPredicate() const
{
    return [this](mx::ConstElementPtr elem)
    {
        if (elem->hasSourceUri())
        {
            return (_xincludeFiles.count(elem->getSourceUri()) == 0);
        }
        return true;
    };
}

void Graph::loadStandardLibraries()
{
    // Initialize the standard library.
    try
    {
        _stdLib = mx::createDocument();
        _xincludeFiles = mx::loadLibraries(_libraryFolders, _searchPath, _stdLib);
        if (_xincludeFiles.empty())
        {
            std::cerr << "Could not find standard data libraries on the given search path: " << _searchPath.asString() << std::endl;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to load standard data libraries: " << e.what() << std::endl;
        return;
    }
}

mx::DocumentPtr Graph::loadDocument(mx::FilePath filename)
{
    mx::FilePathVec libraryFolders = { "libraries" };
    _libraryFolders = libraryFolders;
    mx::XmlReadOptions readOptions;
    readOptions.readXIncludeFunction = [](mx::DocumentPtr doc, const mx::FilePath& filename,
                                          const mx::FileSearchPath& searchPath, const mx::XmlReadOptions* options)
    {
        mx::FilePath resolvedFilename = searchPath.find(filename);
        if (resolvedFilename.exists())
        {
            try
            {
                readFromXmlFile(doc, resolvedFilename, searchPath, options);
            }
            catch (mx::Exception& e)
            {
                std::cerr << "Failed to read include file: " << filename.asString() << ". " <<
                    std::string(e.what()) << std::endl;
            }
        }
        else
        {
            std::cerr << "Include file not found: " << filename.asString() << std::endl;
        }
    };

    mx::DocumentPtr doc = mx::createDocument();
    try
    {
        if (!filename.isEmpty())
        {
            mx::readFromXmlFile(doc, filename, _searchPath, &readOptions);
            std::string message;
            if (!doc->validate(&message))
            {
                std::cerr << "*** Validation warnings for " << filename.asString() << " ***" << std::endl;
                std::cerr << message;
            }
        }
    }
    catch (mx::Exception& e)
    {
        std::cerr << "Failed to read file: " << filename.asString() << ": \"" <<
            std::string(e.what()) << "\"" << std::endl;
    }
    _graphStack = std::stack<std::vector<UiNodePtr>>();
    _pinStack = std::stack<std::vector<Pin>>();
    return doc;
}

// populate nodes to add with input output group and nodegraph nodes which are not found in the stdlib
void Graph::addExtraNodes()
{
    _extraNodes.clear();

    std::vector<std::string> groups{ "Input Nodes", "Output Nodes", "Group Nodes", "Node Graph" };
    std::vector<std::string> types{
        "float", "integer", "vector2", "vector3", "vector4", "color3", "color4", "string", "filename", "bool"
    };
    // need to clear vectors if has previously used tab without there being a document, need to use the current graph doc
    for (std::string group : groups)
    {
        if (_extraNodes[group].size() > 0)
        {
            _extraNodes[group].clear();
        }
    }
    for (std::string type : types)
    {

        std::string nodeName = "ND_input";
        nodeName += type;
        std::vector<std::string> input{ nodeName, type, "input" };
        _extraNodes["Input Nodes"].push_back(input);
        nodeName = "ND_output";
        nodeName += type;
        std::vector<std::string> output{ nodeName, type, "output" };
        _extraNodes["Output Nodes"].push_back(output);
    }
    // group node
    std::vector<std::string> groupNode{ "ND_group", "", "group" };
    _extraNodes["Group Nodes"].push_back(groupNode);
    // node graph nodes
    std::vector<std::string> nodeGraph{ "ND_node graph", "", "nodegraph" };
    _extraNodes["Node Graph"].push_back(nodeGraph);
}

// return output pin needed to link the inputs and outputs
ed::PinId Graph::getOutputPin(UiNodePtr node, UiNodePtr upNode, Pin input)
{
    if (upNode->getNodeGraph() != nullptr)
    {
        // For nodegraph need to get the correct ouput pin accorinding to the names of the output nodes
        mx::OutputPtr output = input._pinNode->getNode()->getConnectedOutput(input._name);
        if (output)
        {
            std::string outName = output->getName();
            for (Pin outputs : upNode->outputPins)
            {
                if (outputs._name == outName)
                {
                    return outputs._pinId;
                }
            }
        }
        return ed::PinId();
    }
    else
    {
        // For node need to get the correct ouput pin based on the output attribute
        if (!upNode->outputPins.empty())
        {
            std::string outputName = mx::EMPTY_STRING;
            if (input._input)
            {
                outputName = input._input->getOutputString();
            }
            else if (input._output)
            {
                outputName = input._output->getOutputString();
            }

            size_t pinIndex = 0;
            if (!outputName.empty())
            {
                for (size_t i = 0; i < upNode->outputPins.size(); i++)
                {
                    if (upNode->outputPins[i]._name == outputName)
                    {
                        pinIndex = i;
                        break;
                    }
                }
            }
            return (upNode->outputPins[pinIndex]._pinId);
        }
        return ed::PinId();
    } 
}

// connect links via connected nodes in UiNodePtr
void Graph::linkGraph()
{
    _currLinks.clear();
    // start with bottom of graph
    for (UiNodePtr node : _graphNodes)
    {
        std::vector<Pin> inputs = node->inputPins;
        if (node->getInput() == nullptr)
        {
            for (size_t i = 0; i < inputs.size(); i++)
            {
                // get upstream node for all inputs
                std::string inputName = inputs[i]._name;

                UiNodePtr inputNode = node->getConnectedNode(inputName);
                if (inputNode != nullptr)
                {
                    Link link;
                    // getting the input connections for the current uiNode
                    ax::NodeEditor::PinId id = inputs[i]._pinId;
                    inputs[i].setConnected(true);
                    int end = int(id.Get());
                    link._endAttr = end;
                    // get id number of output of node

                    ed::PinId outputId = getOutputPin(node, inputNode, inputs[i]);
                    int start = int(outputId.Get());

                    if (start >= 0)
                    {
                        // Connect the correct output pin to this input
                        for (Pin outPin : inputNode->outputPins)
                        {
                            if (outPin._pinId == outputId)
                            {
                                outPin.setConnected(true);
                                outPin.addConnection(inputs[i]);
                            }
                        }

                        link._startAttr = start;

                        if (!linkExists(link))
                        {
                            _currLinks.push_back(link);
                        }
                    }
                }
                else if (inputs[i]._input)
                {
                    if (inputs[i]._input->getInterfaceInput())
                    {

                        inputs[i].setConnected(true);
                    }
                }
                else
                {
                    inputs[i].setConnected(false);
                }
            }
        }
    }
}

// connect all the links via the graph editor library
void Graph::connectLinks()
{

    for (Link const& link : _currLinks)
    {

        ed::Link(link.id, link._startAttr, link._endAttr);
    }
}

// find link position in currLinks vector from link id
int Graph::findLinkPosition(int id)
{

    int count = 0;
    for (size_t i = 0; i < _currLinks.size(); i++)
    {
        if (_currLinks[i].id == id)
        {
            return count;
        }
        count++;
    }
    return -1;
}
// check if a node has already been assigned a position
bool Graph::checkPosition(UiNodePtr node)
{
    if (node->getMxElement() != nullptr)
    {
        if (node->getMxElement()->getAttribute("xpos") != "")
        {
            return true;
        }
    }
    return false;
}
// calculate the total vertical space the node level takes up
float Graph::totalHeight(int level)
{
    float total = 0.f;
    for (UiNodePtr node : _levelMap[level])
    {
        total += ed::GetNodeSize(node->getId()).y;
    }
    return total;
}
// set the y position of node based of the starting position and the nodes above it
void Graph::setYSpacing(int level, float startingPos)
{
    // set the y spacing for each node
    float currPos = startingPos;
    for (UiNodePtr node : _levelMap[level])
    {
        ImVec2 oldPos = ed::GetNodePosition(node->getId());
        ed::SetNodePosition(node->getId(), ImVec2(oldPos.x, currPos));
        currPos += ed::GetNodeSize(node->getId()).y + 40;
    }
}

// calculate the average y position for a specific node level
float Graph::findAvgY(const std::vector<UiNodePtr>& nodes)
{
    // find the mid point of node level grou[
    float total = 0.f;
    int count = 0;
    for (UiNodePtr node : nodes)
    {
        ImVec2 pos = ed::GetNodePosition(node->getId());
        ImVec2 size = ed::GetNodeSize(node->getId());

        total += ((size.y + pos.y) + pos.y) / 2;
        count++;
    }
    return (total / count);
}

void Graph::findYSpacing(float startY)
{
    // assume level 0 is set
    // for each level find the average y position of the previous level to use as a spacing guide
    int i = 0;
    for (std::pair<int, std::vector<UiNodePtr>> levelChunk : _levelMap)
    {
        if (_levelMap[i].size() > 0)
        {
            if (_levelMap[i][0]->_level > 0)
            {

                int prevLevel = _levelMap[i].front()->_level - 1;
                float avgY = findAvgY(_levelMap[prevLevel]);
                float height = totalHeight(_levelMap[i].front()->_level);
                // caculate the starting position to be above the previous level's center so that it is evenly spaced on either side of the center
                float startingPos = avgY - ((height + (_levelMap[i].size() * 20)) / 2) + startY;
                setYSpacing(_levelMap[i].front()->_level, startingPos);
            }
            else
            {
                setYSpacing(_levelMap[i].front()->_level, startY);
            }
        }
        ++i;
    }
}

// layout the x position by assigning the node levels based off its distance from the first node
ImVec2 Graph::layoutPosition(UiNodePtr layoutNode, ImVec2 startingPos, bool initialLayout, int level)
{
    if (checkPosition(layoutNode) && !_autoLayout)
    {
        for (UiNodePtr node : _graphNodes)
        {
            // since nodegrpah nodes do not have any materialX info they are placed based off their conneced node
            if (node->getNodeGraph() != nullptr)
            {
                std::vector<UiNodePtr> outputCon = node->getOutputConnections();
                if (outputCon.size() > 0)
                {
                    ImVec2 outputPos = ed::GetNodePosition(outputCon[0]->getId());
                    ed::SetNodePosition(node->getId(), ImVec2(outputPos.x - 400, outputPos.y));
                    node->setPos(ImVec2(outputPos.x - 400, outputPos.y));
                }
            }
            else
            {
                // don't set position of group nodes
                if (node->getMessage() == "")
                {
                    float x = std::stof(node->getMxElement()->getAttribute("xpos"));
                    float y = std::stof(node->getMxElement()->getAttribute("ypos"));
                    x *= DEFAULT_NODE_SIZE.x;
                    y *= DEFAULT_NODE_SIZE.y;
                    ed::SetNodePosition(node->getId(), ImVec2(x, y));
                    node->setPos(ImVec2(x, y));
                }
            }
        }
        return ImVec2(0.f, 0.f);
    }
    else
    {
        ImVec2 currPos = startingPos;
        ImVec2 newPos = currPos;
        if (layoutNode->_level != -1)
        {
            if (layoutNode->_level < level)
            {
                // remove the old instance of the node from the map
                int levelNum = 0;
                int removeNum = -1;
                for (UiNodePtr levelNode : _levelMap[layoutNode->_level])
                {
                    if (levelNode->getName() == layoutNode->getName())
                    {
                        removeNum = levelNum;
                    }
                    levelNum++;
                }
                if (removeNum > -1)
                {
                    _levelMap[layoutNode->_level].erase(_levelMap[layoutNode->_level].begin() + removeNum);
                }

                layoutNode->_level = level;
            }
        }
        else
        {
            layoutNode->_level = level;
        }

        auto it = _levelMap.find(layoutNode->_level);
        if (it != _levelMap.end())
        {
            // key already exists add to it
            bool nodeFound = false;
            for (UiNodePtr node : it->second)
            {
                if (node && node->getName() == layoutNode->getName())
                {
                    nodeFound = true;
                    break;
                }
            }
            if (!nodeFound)
            {
                _levelMap[layoutNode->_level].push_back(layoutNode);
            }
        }
        else
        {
            // insert new vector into key
            std::vector<UiNodePtr> newValue = { layoutNode };
            _levelMap.insert({ layoutNode->_level, newValue });
        }
        std::vector<Pin> pins = layoutNode->inputPins;
        if (initialLayout)
        {
            // check number of inputs that are connected to node
            if (layoutNode->getInputConnect() > 0)
            {
                // not top of node graph stop recursion
                if (pins.size() != 0 && layoutNode->getInput() == nullptr)
                {
                    int numNode = 0;
                    for (size_t i = 0; i < pins.size(); i++)
                    {
                        // get upstream node for all inputs
                        newPos = startingPos;
                        UiNodePtr nextNode = layoutNode->getConnectedNode(pins[i]._name);
                        if (nextNode)
                        {
                            startingPos.x = 1200.f - ((layoutNode->_level) * 350);
                            // pos.y = 0;
                            ed::SetNodePosition(layoutNode->getId(), startingPos);
                            layoutNode->setPos(ImVec2(startingPos));

                            newPos.x = 1200.f - ((layoutNode->_level + 1) * 75);
                            ++numNode;
                            // call layout position on upstream node with newPos as -140 to the left of current node
                            layoutPosition(nextNode, ImVec2(newPos.x, startingPos.y), initialLayout, layoutNode->_level + 1);
                        }
                    }
                }
            }
            else
            {
                startingPos.x = 1200.f - ((layoutNode->_level) * 350);
                layoutNode->setPos(ImVec2(startingPos));
                // set current node position
                ed::SetNodePosition(layoutNode->getId(), ImVec2(startingPos));
            }
        }
        return ImVec2(0.f, 0.f);
    }
}

// extra layout pass for inputs and nodes that do not attach to an output node
void Graph::layoutInputs()
{
    // layout inputs after other nodes so that they can be all in a line on far left side of node graph
    if (_levelMap.begin() != _levelMap.end())
    {
        int levelCount = -1;
        for (std::pair<int, std::vector<UiNodePtr>> nodes : _levelMap)
        {
            ++levelCount;
        }
        ImVec2 startingPos = ed::GetNodePosition(_levelMap[levelCount].back()->getId());
        startingPos.y += ed::GetNodeSize(_levelMap[levelCount].back()->getId()).y + 20;

        for (UiNodePtr uiNode : _graphNodes)
        {

            if (uiNode->getOutputConnections().size() == 0 && (uiNode->getInput() != nullptr))
            {
                ed::SetNodePosition(uiNode->getId(), ImVec2(startingPos));
                startingPos.y += ed::GetNodeSize(uiNode->getId()).y;
                startingPos.y += 23;
            }
            // accoutning for extra nodes like in gltf
            else if (uiNode->getOutputConnections().size() == 0 && (uiNode->getNode() != nullptr))
            {
                if (uiNode->getNode()->getCategory() != mx::SURFACE_MATERIAL_NODE_STRING)
                {
                    layoutPosition(uiNode, ImVec2(1200, 750), _initial, 0);
                }
            }
        }
    }
}

// reutrn pin color based on the type of the value of that pin
void Graph::setPinColor()
{
    _pinColor.insert(std::make_pair("integer", ImColor(255, 255, 28, 255)));
    _pinColor.insert(std::make_pair("boolean", ImColor(255, 0, 255, 255)));
    _pinColor.insert(std::make_pair("float", ImColor(50, 100, 255, 255)));
    _pinColor.insert(std::make_pair("color3", ImColor(178, 34, 34, 255)));
    _pinColor.insert(std::make_pair("color4", ImColor(50, 10, 255, 255)));
    _pinColor.insert(std::make_pair("vector2", ImColor(100, 255, 100, 255)));
    _pinColor.insert(std::make_pair("vector3", ImColor(0, 255, 0, 255)));
    _pinColor.insert(std::make_pair("vector4", ImColor(100, 0, 100, 255)));
    _pinColor.insert(std::make_pair("matrix33", ImColor(0, 100, 100, 255)));
    _pinColor.insert(std::make_pair("matrix44", ImColor(50, 255, 100, 255)));
    _pinColor.insert(std::make_pair("filename", ImColor(255, 184, 28, 255)));
    _pinColor.insert(std::make_pair("string", ImColor(100, 100, 50, 255)));
    _pinColor.insert(std::make_pair("geomname", ImColor(121, 60, 180, 255)));
    _pinColor.insert(std::make_pair("BSDF", ImColor(10, 181, 150, 255)));
    _pinColor.insert(std::make_pair("EDF", ImColor(255, 50, 100, 255)));
    _pinColor.insert(std::make_pair("VDF", ImColor(0, 100, 151, 255)));
    _pinColor.insert(std::make_pair("surfaceshader", ImColor(150, 255, 255, 255)));
    _pinColor.insert(std::make_pair("material", ImColor(255, 255, 255, 255)));
    _pinColor.insert(std::make_pair(mx::DISPLACEMENT_SHADER_TYPE_STRING, ImColor(155, 50, 100, 255)));
    _pinColor.insert(std::make_pair(mx::VOLUME_SHADER_TYPE_STRING, ImColor(155, 250, 100, 255)));
    _pinColor.insert(std::make_pair(mx::LIGHT_SHADER_TYPE_STRING, ImColor(100, 150, 100, 255)));
    _pinColor.insert(std::make_pair("none", ImColor(140, 70, 70, 255)));
    _pinColor.insert(std::make_pair(mx::MULTI_OUTPUT_TYPE_STRING, ImColor(70, 70, 70, 255)));
    _pinColor.insert(std::make_pair("integerarray", ImColor(200, 10, 100, 255)));
    _pinColor.insert(std::make_pair("floatarray", ImColor(25, 250, 100)));
    _pinColor.insert(std::make_pair("color3array", ImColor(25, 200, 110)));
    _pinColor.insert(std::make_pair("color4array", ImColor(50, 240, 110)));
    _pinColor.insert(std::make_pair("vector2array", ImColor(50, 200, 75)));
    _pinColor.insert(std::make_pair("vector3array", ImColor(20, 200, 100)));
    _pinColor.insert(std::make_pair("vector4array", ImColor(100, 200, 100)));
    _pinColor.insert(std::make_pair("geomnamearray", ImColor(150, 200, 100)));
    _pinColor.insert(std::make_pair("stringarray", ImColor(120, 180, 100)));
}

// based off of showLabel from ImGui Node Editor blueprints-example.cpp
auto showLabel = [](const char* label, ImColor color)
{
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
    auto size = ImGui::CalcTextSize(label);

    auto padding = ImGui::GetStyle().FramePadding;
    auto spacing = ImGui::GetStyle().ItemSpacing;

    ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

    auto rectMin = ImGui::GetCursorScreenPos() - padding;
    auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

    auto drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
    ImGui::TextUnformatted(label);
};

void Graph::selectMaterial(UiNodePtr uiNode)
{
    // find renderable element that correspond with material uiNode
    std::vector<mx::TypedElementPtr> elems;
    mx::findRenderableElements(_graphDoc, elems);
    mx::TypedElementPtr typedElem = nullptr;
    for (mx::TypedElementPtr elem : elems)
    {
        mx::TypedElementPtr renderableElem = elem;
        mx::NodePtr node = elem->asA<mx::Node>();
        if (node == uiNode->getNode())
        {
            typedElem = elem;
        }
    }
    _renderer->setMaterial(typedElem);
}

// set the node to display in render veiw based off the selected node or nodegraph
void Graph::setRenderMaterial(UiNodePtr node)
{
    // set render node right away is node is a material
    if (node->getNode() && node->getNode()->getType() == "material")
    {
        // only set new render node if different material has been selected
        if (_currRenderNode != node)
        {
            _currRenderNode = node;
            _frameCount = ImGui::GetFrameCount();
            _renderer->setMaterialCompilation(true);
        }
    }
    else
    {
        // continue downstream using output connections until a material node is found
        std::vector<UiNodePtr> outNodes = node->getOutputConnections();
        if (outNodes.size() > 0)
        {
            if (outNodes[0]->getNode())
            {
                if (outNodes[0]->getNode()->getType() == mx::SURFACE_SHADER_TYPE_STRING)
                {
                    std::vector<UiNodePtr> shaderOut = outNodes[0]->getOutputConnections();
                    if (shaderOut.size() > 0)
                    {
                        if (shaderOut[0])
                        {
                            if (shaderOut[0]->getNode()->getType() == "material")
                            {
                                if (_currRenderNode != shaderOut[0])
                                {
                                    _currRenderNode = shaderOut[0];
                                    _frameCount = ImGui::GetFrameCount();
                                    _renderer->setMaterialCompilation(true);
                                }
                            }
                        }
                    }
                    else
                    {
                        _currRenderNode = nullptr;
                    }
                }
                else if (outNodes[0]->getNode()->getType() == mx::MATERIAL_TYPE_STRING)
                {
                    if (_currRenderNode != outNodes[0])
                    {
                        _currRenderNode = outNodes[0];
                        _frameCount = ImGui::GetFrameCount();
                        _renderer->setMaterialCompilation(true);
                    }
                }
            }
            else
            {
                _currRenderNode = nullptr;
            }
        }
        else
        {
            _currRenderNode = nullptr;
        }
    }
}

void Graph::updateMaterials(mx::InputPtr input, mx::ValuePtr value)
{
    std::string renderablePath;
    std::vector<mx::TypedElementPtr> elems;
    mx::TypedElementPtr renderableElem;
    mx::findRenderableElements(_graphDoc, elems);

    size_t num = 0;
    int num2 = 0;
    for (mx::TypedElementPtr elem : elems)
    {
        renderableElem = elem;
        mx::NodePtr node = elem->asA<mx::Node>();
        if (node)
        {
            if (_currRenderNode)
            {
                if (node->getName() == _currRenderNode->getName())
                {
                    renderablePath = renderableElem->getNamePath();
                    break;
                }
            }
            else
            {
                renderablePath = renderableElem->getNamePath();
            }
        }
        else
        {
            renderablePath = renderableElem->getNamePath();
            if (num2 == 2)
            {
                break;
            }
            num2++;
        }
    }

    if (renderablePath.empty())
    {
        _renderer->updateMaterials(nullptr);
    }
    else
    {
        if (!input)
        {
            mx::ElementPtr elem = nullptr;
            {
                elem = _graphDoc->getDescendant(renderablePath);
            }
            mx::TypedElementPtr typedElem = elem ? elem->asA<mx::TypedElement>() : nullptr;
            _renderer->updateMaterials(typedElem);
        }
        else
        {
            std::string name = input->getNamePath();
            // need to use exact interface name in order for input
            mx::InputPtr interfaceInput = findInput(input, input->getName());
            if (interfaceInput)
            {
                name = interfaceInput->getNamePath();
            }
            // Note that if there is a topogical change due to
            // this value change or a transparency change, then
            // this is not currently caught here.
            _renderer->getMaterials()[num]->modifyUniform(name, value);
        }
    }
}
// set the value of the selected node constants in the node property editor
void Graph::setConstant(UiNodePtr node, mx::InputPtr& input)
{
    std::string inName = input->getName();
    float labelWidth = ImGui::CalcTextSize(inName.c_str()).x;
    // if input is a float set the float slider Ui to the value
    if (input->getType() == "float")
    {
        mx::ValuePtr val = input->getValue();

        if (val && val->isA<float>())
        {
            // updates the value to the default for new nodes
            float prev = val->asA<float>(), temp = val->asA<float>();
            ImGui::SameLine();
            ImGui::PushItemWidth(labelWidth + 20);
            ImGui::DragFloat("##hidelabel", &temp, 0.01f, 0.f, 100.f);
            ImGui::PopItemWidth();
            // set input value  and update materials if different from previous value
            if (prev != temp)
            {
                addNodeInput(_currUiNode, input);
                input->setValue(temp, input->getType());
                updateMaterials(input, input->getValue());
            }
        }
    }
    else if (input->getType() == "integer")
    {
        mx::ValuePtr val = input->getValue();
        if (val && val->isA<int>())
        {
            int prev = val->asA<int>(), temp = val->asA<int>();
            ImGui::SameLine();
            ImGui::PushItemWidth(labelWidth + 20);
            ImGui::DragInt("##hidelabel", &temp, 1, 0, 100);
            ImGui::PopItemWidth();
            // set input value  and update materials if different from previous value
            if (prev != temp)
            {
                addNodeInput(_currUiNode, input);
                input->setValue(temp, input->getType());
                updateMaterials(input, input->getValue());
            }
        }
    }
    else if (input->getType() == "color3")
    {
        mx::ValuePtr val = input->getValue();
        if (val && val->isA<mx::Color3>())
        {
            mx::Color3 prev = val->asA<mx::Color3>(), temp = val->asA<mx::Color3>();
            ImGui::SameLine();
            ImGui::PushItemWidth(labelWidth + 100);
            ImGui::DragFloat3("##hidelabel", &temp[0], 0.01f, 0.f, 100.f);
            ImGui::SameLine();
            ImGui::ColorEdit3("##color", &temp[0], ImGuiColorEditFlags_NoInputs);
            ImGui::PopItemWidth();

            // set input value  and update materials if different from previous value
            if (prev != temp)
            {
                addNodeInput(_currUiNode, input);
                input->setValue(temp, input->getType());
                updateMaterials(input, input->getValue());
            }
        }
    }
    else if (input->getType() == "color4")
    {
        mx::ValuePtr val = input->getValue();
        if (val && val->isA<mx::Color4>())
        {
            mx::Color4 prev = val->asA<mx::Color4>(), temp = val->asA<mx::Color4>();
            ImGui::SameLine();
            ImGui::PushItemWidth(labelWidth + 100);
            ImGui::DragFloat4("##hidelabel", &temp[0], 0.01f, 0.f, 100.f);
            ImGui::SameLine();
            // color edit for the color picker to the right of the color floats
            ImGui::ColorEdit4("##color", &temp[0], ImGuiColorEditFlags_NoInputs);
            ImGui::PopItemWidth();
            // set input value  and update materials if different from previous value
            if (temp != prev)
            {
                addNodeInput(_currUiNode, input);
                input->setValue(temp, input->getType());
                updateMaterials(input, input->getValue());
            }
        }
    }
    else if (input->getType() == "vector2")
    {
        mx::ValuePtr val = input->getValue();
        if (val && val->isA<mx::Vector2>())
        {
            mx::Vector2 prev = val->asA<mx::Vector2>(), temp = val->asA<mx::Vector2>();
            ImGui::SameLine();
            ImGui::PushItemWidth(labelWidth + 100);
            ImGui::DragFloat2("##hidelabel", &temp[0], 0.01f, 0.f, 100.f);
            ImGui::PopItemWidth();
            // set input value  and update materials if different from previous value
            if (prev != temp)
            {
                addNodeInput(_currUiNode, input);
                input->setValue(temp, input->getType());
                updateMaterials(input, input->getValue());
            }
        }
    }
    else if (input->getType() == "vector3")
    {
        mx::ValuePtr val = input->getValue();
        if (val && val->isA<mx::Vector3>())
        {
            mx::Vector3 prev = val->asA<mx::Vector3>(), temp = val->asA<mx::Vector3>();
            ImGui::SameLine();
            ImGui::PushItemWidth(labelWidth + 100);
            ImGui::DragFloat3("##hidelabel", &temp[0], 0.01f, 0.f, 100.f);
            ImGui::PopItemWidth();
            // set input value  and update materials if different from previous value
            if (prev != temp)
            {
                addNodeInput(_currUiNode, input);
                input->setValue(temp, input->getType());
                updateMaterials(input, input->getValue());
            }
        }
    }
    else if (input->getType() == "vector4")
    {
        mx::ValuePtr val = input->getValue();
        if (val && val->isA<mx::Vector4>())
        {
            mx::Vector4 prev = val->asA<mx::Vector4>(), temp = val->asA<mx::Vector4>();
            ImGui::SameLine();
            ImGui::PushItemWidth(labelWidth + 90);
            ImGui::DragFloat4("##hidelabel", &temp[0], 0.01f, 0.f, 100.f);
            ImGui::PopItemWidth();
            // set input value  and update materials if different from previous value
            if (prev != temp)
            {
                addNodeInput(_currUiNode, input);
                input->setValue(temp, input->getType());
                updateMaterials(input, input->getValue());
            }
        }
    }
    else if (input->getType() == "string")
    {
        mx::ValuePtr val = input->getValue();
        if (val && val->isA<std::string>())
        {
            std::string prev = val->asA<std::string>(), temp = val->asA<std::string>();
            ImGui::SameLine();
            ImGui::PushItemWidth(labelWidth);
            ImGui::InputText("##constant", &temp);
            ImGui::PopItemWidth();
            // set input value  and update materials if different from previous value
            if (prev != temp)
            {
                addNodeInput(_currUiNode, input);
                input->setValue(temp, input->getType());
                updateMaterials();
            }
        }
    }
    else if (input->getType() == "filename")
    {
        mx::ValuePtr val = input->getValue();

        if (val && val->isA<std::string>())
        {
            std::string temp = val->asA<std::string>(), prev = val->asA<std::string>();
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.15f, .15f, .15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.2f, .4f, .6f, 1.0f));
            // browser button to select new file
            if (ImGui::Button("Browse"))
            {
                _fileDialogConstant.SetTitle("Node Input Dialog");
                _fileDialogConstant.Open();
                mx::StringSet supportedExtensions = _renderer ? _renderer->getImageHandler()->supportedExtensions() : mx::StringSet();
                std::vector<std::string> filters;
                for (const std::string& supportedExtension : supportedExtensions)
                {
                    filters.push_back("." + supportedExtension);
                }
                _fileDialogConstant.SetTypeFilters(filters);
            }
            ImGui::SameLine();
            ImGui::PushItemWidth(labelWidth);
            ImGui::Text("%s", mx::FilePath(temp).getBaseName().c_str());
            ImGui::PopItemWidth();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();

            // create and load document from selected file
            if (_fileDialogConstant.HasSelected())
            {
                // set the new filename to the complete file path
                mx::FilePath fileName = mx::FilePath(_fileDialogConstant.GetSelected().string());
                temp = fileName;
                // need to set the file prefix for the input to "" so that it can find the new file
                input->setAttribute(input->FILE_PREFIX_ATTRIBUTE, "");
                _fileDialogConstant.ClearSelected();
                _fileDialogConstant.SetTypeFilters(std::vector<std::string>());
            }

            // set input value  and update materials if different from previous value
            if (prev != temp)
            {
                addNodeInput(_currUiNode, input);
                input->setValueString(temp);
                input->setValue(temp, input->getType());
                updateMaterials();
            }
        }
    }
    else if (input->getType() == "boolean")
    {
        mx::ValuePtr val = input->getValue();
        if (val && val->isA<bool>())
        {
            bool prev = val->asA<bool>(), temp = val->asA<bool>();
            ImGui::SameLine();
            ImGui::PushItemWidth(labelWidth);
            ImGui::Checkbox("", &temp);
            ImGui::PopItemWidth();
            // set input value  and update materials if different from previous value
            if (prev != temp)
            {
                addNodeInput(_currUiNode, input);
                input->setValue(temp, input->getType());
                updateMaterials(input, input->getValue());
            }
        }
    }
}
// build the initial graph of a loaded mtlx document including shader, material and nodegraph node
void Graph::setUiNodeInfo(UiNodePtr node, std::string type, std::string category)
{
    node->setType(type);
    node->setCategory(category);
    ++_graphTotalSize;
    // create pins
    if (node->getNodeGraph())
    {
        std::vector<mx::OutputPtr> outputs = node->getNodeGraph()->getOutputs();
        for (mx::OutputPtr out : outputs)
        {
            Pin outPin = Pin(_graphTotalSize, &*out->getName().begin(), out->getType(), node, ax::NodeEditor::PinKind::Output, nullptr, nullptr);
            ++_graphTotalSize;
            node->outputPins.push_back(outPin);
            _currPins.push_back(outPin);
        }

        for (mx::InputPtr input : node->getNodeGraph()->getInputs())
        {
            Pin inPin = Pin(_graphTotalSize, &*input->getName().begin(), input->getType(), node, ax::NodeEditor::PinKind::Input, input, nullptr);
            node->inputPins.push_back(inPin);
            _currPins.push_back(inPin);
            ++_graphTotalSize;
        }
    }
    else
    {
        if (node->getNode())
        {
            mx::NodeDefPtr nodeDef = node->getNode()->getNodeDef(node->getNode()->getName());
            if (nodeDef)
            {
                for (mx::InputPtr input : nodeDef->getActiveInputs())
                {
                    if (node->getNode()->getInput(input->getName()))
                    {
                        input = node->getNode()->getInput(input->getName());
                    }
                    Pin inPin = Pin(_graphTotalSize, &*input->getName().begin(), input->getType(), node, ax::NodeEditor::PinKind::Input, input, nullptr);
                    node->inputPins.push_back(inPin);
                    _currPins.push_back(inPin);
                    ++_graphTotalSize;                    
                }

                for (mx::OutputPtr output : nodeDef->getActiveOutputs())
                {
                    if (node->getNode()->getOutput(output->getName()))
                    {
                        output = node->getNode()->getOutput(output->getName());
                    }
                    Pin outPin = Pin(_graphTotalSize, &*output->getName().begin(), 
                    output->getType(), node, ax::NodeEditor::PinKind::Output, nullptr, nullptr);
                    node->outputPins.push_back(outPin);
                    _currPins.push_back(outPin);
                    ++_graphTotalSize;                    
                }
            }
        }
        else if (node->getInput())
        {
            Pin inPin = Pin(_graphTotalSize, &*("Value"), node->getInput()->getType(), node, ax::NodeEditor::PinKind::Input, node->getInput(), nullptr);
            node->inputPins.push_back(inPin);
            _currPins.push_back(inPin);
            ++_graphTotalSize;
        }
        else if (node->getOutput())
        {
            Pin inPin = Pin(_graphTotalSize, &*("input"), node->getOutput()->getType(), node, ax::NodeEditor::PinKind::Input, nullptr, node->getOutput());
            node->inputPins.push_back(inPin);
            _currPins.push_back(inPin);
            ++_graphTotalSize;
        }

        if (node->getInput() || node->getOutput())
        {
            Pin outPin = Pin(_graphTotalSize, &*("output"), type, node, ax::NodeEditor::PinKind::Output, nullptr, nullptr);
            ++_graphTotalSize;
            node->outputPins.push_back(outPin);
            _currPins.push_back(outPin);
        }
    }

    _graphNodes.push_back(std::move(node));
}

// Generate node UI from nodedefs.
void Graph::createNodeUIList(mx::DocumentPtr doc)
{
    _nodesToAdd.clear();
    const std::string EXTRA_GROUP_NAME = "extra";
    for (mx::NodeDefPtr nodeDef : doc->getNodeDefs())
    {
        // nodeDef is the key for the map
        std::string group = nodeDef->getNodeGroup();
        if (group.empty())
        {
            group = EXTRA_GROUP_NAME;
        }
        if (_nodesToAdd.find(group) == _nodesToAdd.end())
        {
            _nodesToAdd[group] = std::vector<mx::NodeDefPtr>();
        }
        _nodesToAdd[group].push_back(nodeDef);
    }

    addExtraNodes();
}

// build the UiNode node graph based off of loading a document
void Graph::buildUiBaseGraph(mx::DocumentPtr doc)
{
    std::vector<mx::NodeGraphPtr> nodeGraphs = doc->getNodeGraphs();
    std::vector<mx::InputPtr> inputNodes = doc->getActiveInputs();
    std::vector<mx::OutputPtr> outputNodes = doc->getOutputs();
    std::vector<mx::NodePtr> docNodes = doc->getNodes();

    mx::ElementPredicate includeElement = getElementPredicate();

    _graphNodes.clear();
    _currLinks.clear();
    _currEdge.clear();
    _newLinks.clear();
    _currPins.clear();
    _graphTotalSize = 1;
    // creating uiNodes for nodes that belong to the document so they are not in a nodegraph
    for (mx::NodePtr node : docNodes)
    {
        if (!includeElement(node))
            continue;
        std::string name = node->getName();
        auto currNode = std::make_shared<UiNode>(name, _graphTotalSize);
        currNode->setNode(node);
        setUiNodeInfo(currNode, node->getType(), node->getCategory());
    }
    // creating uiNodes for the nodegraph
    for (mx::NodeGraphPtr nodeGraph : nodeGraphs)
    {
        if (!includeElement(nodeGraph))
            continue;
        std::string name = nodeGraph->getName();
        auto currNode = std::make_shared<UiNode>(name, _graphTotalSize);
        currNode->setNodeGraph(nodeGraph);
        setUiNodeInfo(currNode, "", "nodegraph");
    }
    for (mx::InputPtr input : inputNodes)
    {
        if (!includeElement(input))
            continue;
        auto currNode = std::make_shared<UiNode>(input->getName(), _graphTotalSize);
        currNode->setInput(input);
        setUiNodeInfo(currNode, input->getType(), input->getCategory());
    }
    for (mx::OutputPtr output : outputNodes)
    {
        if (!includeElement(output))
            continue;
        auto currNode = std::make_shared<UiNode>(output->getName(), _graphTotalSize);
        currNode->setOutput(output);
        setUiNodeInfo(currNode, output->getType(), output->getCategory());
    }
    // creating edges for nodegraphs
    for (mx::NodeGraphPtr graph : nodeGraphs)
    {
        for (mx::InputPtr input : graph->getActiveInputs())
        {
            int downNum = -1;
            int upNum = -1;
            mx::NodePtr connectedNode = input->getConnectedNode();
            if (connectedNode)
            {
                downNum = findNode(graph->getName(), "nodegraph");
                upNum = findNode(connectedNode->getName(), "node");
                if (upNum > -1)
                {
                    UiEdge newEdge = UiEdge(_graphNodes[upNum], _graphNodes[downNum], input);
                    if (!edgeExists(newEdge))
                    {
                        _graphNodes[downNum]->edges.push_back(newEdge);
                        _graphNodes[downNum]->setInputNodeNum(1);
                        _graphNodes[upNum]->setOutputConnection(_graphNodes[downNum]);
                        _currEdge.push_back(newEdge);
                    }
                }
            }
        }
    }
    // creating edges for surface and material nodes
    for (mx::NodePtr node : docNodes)
    {
        mx::NodeDefPtr nD = node->getNodeDef(node->getName());
        for (mx::InputPtr input : node->getActiveInputs())
        {

            mx::string nodeGraphName = input->getNodeGraphString();
            mx::NodePtr connectedNode = input->getConnectedNode();
            mx::OutputPtr connectedOutput = input->getConnectedOutput();
            int upNum = -1;
            int downNum = -1;
            if (nodeGraphName != "")
            {

                upNum = findNode(nodeGraphName, "nodegraph");
                downNum = findNode(node->getName(), "node");
            }
            else if (connectedNode)
            {
                upNum = findNode(connectedNode->getName(), "node");
                downNum = findNode(node->getName(), "node");
            }
            else if (connectedOutput)
            {
                upNum = findNode(connectedOutput->getName(), "output");
                downNum = findNode(node->getName(), "node");
            }
            else if (input->getInterfaceName() != "")
            {
                upNum = findNode(input->getInterfaceName(), "input");
                downNum = findNode(node->getName(), "node");
            }
            if (upNum != -1)
            {

                UiEdge newEdge = UiEdge(_graphNodes[upNum], _graphNodes[downNum], input);
                if (!edgeExists(newEdge))
                {
                    _graphNodes[downNum]->edges.push_back(newEdge);
                    _graphNodes[downNum]->setInputNodeNum(1);
                    _graphNodes[upNum]->setOutputConnection(_graphNodes[downNum]);
                    _currEdge.push_back(newEdge);
                }
            }
        }
    }
}
// build the UiNode node graph based off of diving into a node graph node
void Graph::buildUiNodeGraph(const mx::NodeGraphPtr& nodeGraphs)
{

    // clear all values so that ids can start with 0 or 1
    _graphNodes.clear();
    _currLinks.clear();
    _currEdge.clear();
    _newLinks.clear();
    _currPins.clear();
    _graphTotalSize = 1;
    if (nodeGraphs)
    {
        mx::NodeGraphPtr nodeGraph = nodeGraphs;
        std::vector<mx::ElementPtr> children = nodeGraph->topologicalSort();
        // Write out all nodes.

        mx::NodeDefPtr nodeDef = nodeGraph->getNodeDef();
        mx::NodeDefPtr currNodeDef;

        // create input nodes
        if (nodeDef)
        {
            std::vector<mx::InputPtr> inputs = nodeDef->getActiveInputs();

            for (mx::InputPtr input : inputs)
            {
                auto currNode = std::make_shared<UiNode>(input->getName(), _graphTotalSize);
                currNode->setInput(input);
                setUiNodeInfo(currNode, input->getType(), input->getCategory());
            }
        }

        // search node graph children to create uiNodes
        for (mx::ElementPtr elem : children)
        {
            mx::NodePtr node = elem->asA<mx::Node>();
            mx::InputPtr input = elem->asA<mx::Input>();
            mx::OutputPtr output = elem->asA<mx::Output>();
            std::string name = elem->getName();
            auto currNode = std::make_shared<UiNode>(name, _graphTotalSize);
            if (node)
            {
                currNode->setNode(node);
                setUiNodeInfo(currNode, node->getType(), node->getCategory());
            }
            else if (input)
            {
                currNode->setInput(input);
                setUiNodeInfo(currNode, input->getType(), input->getCategory());
            }
            else if (output)
            {
                currNode->setOutput(output);
                setUiNodeInfo(currNode, output->getType(), output->getCategory());
            }
        }

        // Write out all connections.
        std::set<mx::Edge> processedEdges;
        for (mx::OutputPtr output : nodeGraph->getOutputs())
        {
            for (mx::Edge edge : output->traverseGraph())
            {
                if (!processedEdges.count(edge))
                {
                    mx::ElementPtr upstreamElem = edge.getUpstreamElement();
                    mx::ElementPtr downstreamElem = edge.getDownstreamElement();
                    mx::ElementPtr connectingElem = edge.getConnectingElement();

                    mx::NodePtr upstreamNode = upstreamElem->asA<mx::Node>();
                    mx::NodePtr downstreamNode = downstreamElem->asA<mx::Node>();
                    mx::InputPtr upstreamInput = upstreamElem->asA<mx::Input>();
                    mx::InputPtr downstreamInput = downstreamElem->asA<mx::Input>();
                    mx::OutputPtr upstreamOutput = upstreamElem->asA<mx::Output>();
                    mx::OutputPtr downstreamOutput = downstreamElem->asA<mx::Output>();
                    std::string downName = downstreamElem->getName();
                    std::string upName = upstreamElem->getName();
                    std::string upstreamType;
                    std::string downstreamType;
                    if (upstreamNode)
                    {
                        upstreamType = "node";
                    }
                    else if (upstreamInput)
                    {
                        upstreamType = "input";
                    }
                    else if (upstreamOutput)
                    {
                        upstreamType = "output";
                    }
                    if (downstreamNode)
                    {
                        downstreamType = "node";
                    }
                    else if (downstreamInput)
                    {
                        downstreamType = "input";
                    }
                    else if (downstreamOutput)
                    {
                        downstreamType = "output";
                    }
                    int upNode = findNode(upName, upstreamType);
                    int downNode = findNode(downName, downstreamType);
                    if (downNode > 0 && upNode > 0 && 
                        _graphNodes[downNode]->getOutput() != nullptr)
                    {
                        // creating edges for the output nodes
                        UiEdge newEdge = UiEdge(_graphNodes[upNode], _graphNodes[downNode], nullptr);
                        if (!edgeExists(newEdge))
                        {
                            _graphNodes[downNode]->edges.push_back(newEdge);
                            _graphNodes[downNode]->setInputNodeNum(1);
                            _graphNodes[upNode]->setOutputConnection(_graphNodes[downNode]);
                            _currEdge.push_back(newEdge);
                        }
                    }
                    else if (connectingElem)
                    {

                        mx::InputPtr connectingInput = connectingElem->asA<mx::Input>();

                        if (connectingInput)
                        {
                            if ((upNode >= 0) && (downNode >= 0))
                            {
                                UiEdge newEdge = UiEdge(_graphNodes[upNode], _graphNodes[downNode], connectingInput);
                                if (!edgeExists(newEdge))
                                {
                                    _graphNodes[downNode]->edges.push_back(newEdge);
                                    _graphNodes[downNode]->setInputNodeNum(1);
                                    _graphNodes[upNode]->setOutputConnection(_graphNodes[downNode]);
                                    _currEdge.push_back(newEdge);
                                }
                            }
                        }
                    }
                    if (upstreamNode)
                    {
                        std::vector<mx::InputPtr> ins = upstreamNode->getActiveInputs();
                        for (mx::InputPtr input : ins)
                        {
                            // connecting input nodes
                            if (input->hasInterfaceName())
                            {
                                std::string interfaceName = input->getInterfaceName();
                                int newUp = findNode(interfaceName, "input");
                                if (newUp >= 0)
                                {
                                    mx::InputPtr inputP = std::make_shared<mx::Input>(downstreamElem, input->getName());
                                    UiEdge newEdge = UiEdge(_graphNodes[newUp], _graphNodes[upNode], input);
                                    if (!edgeExists(newEdge))
                                    {
                                        _graphNodes[upNode]->edges.push_back(newEdge);
                                        _graphNodes[upNode]->setInputNodeNum(1);
                                        _graphNodes[newUp]->setOutputConnection(_graphNodes[upNode]);
                                        _currEdge.push_back(newEdge);
                                    }
                                }
                            }
                        }
                    }

                    processedEdges.insert(edge);
                }
            }
        }

        // second pass to catch all of the connections that arent part of an output
        for (mx::ElementPtr elem : children)
        {
            mx::NodePtr node = elem->asA<mx::Node>();
            mx::InputPtr inputElem = elem->asA<mx::Input>();
            mx::OutputPtr output = elem->asA<mx::Output>();
            if (node)
            {
                std::vector<mx::InputPtr> inputs = node->getActiveInputs();
                for (mx::InputPtr input : inputs)
                {
                    mx::NodePtr upNode = input->getConnectedNode();
                    if (upNode)
                    {
                        int upNum = findNode(upNode->getName(), "node");
                        int downNode = findNode(node->getName(), "node");
                        if ((upNum >= 0) && (downNode >= 0))
                        {

                            UiEdge newEdge = UiEdge(_graphNodes[upNum], _graphNodes[downNode], input);
                            if (!edgeExists(newEdge))
                            {
                                _graphNodes[downNode]->edges.push_back(newEdge);
                                _graphNodes[downNode]->setInputNodeNum(1);
                                _graphNodes[upNum]->setOutputConnection(_graphNodes[downNode]);
                                _currEdge.push_back(newEdge);
                            }
                        }
                    }
                    else if (input->getInterfaceInput())
                    {
                        int upNum = findNode(input->getInterfaceInput()->getName(), "input");
                        int downNode = findNode(node->getName(), "node");
                        if ((upNum >= 0) && (downNode >= 0))
                        {

                            UiEdge newEdge = UiEdge(_graphNodes[upNum], _graphNodes[downNode], input);
                            if (!edgeExists(newEdge))
                            {
                                _graphNodes[downNode]->edges.push_back(newEdge);
                                _graphNodes[downNode]->setInputNodeNum(1);
                                _graphNodes[upNum]->setOutputConnection(_graphNodes[downNode]);
                                _currEdge.push_back(newEdge);
                            }
                        }
                    }
                }
            }
            else if (output)
            {
                mx::NodePtr upNode = output->getConnectedNode();
                if (upNode)
                {
                    int upNum = findNode(upNode->getName(), "node");
                    int downNode = findNode(output->getName(), "output");
                    UiEdge newEdge = UiEdge(_graphNodes[upNum], _graphNodes[downNode], nullptr);
                    if (!edgeExists(newEdge))
                    {
                        _graphNodes[downNode]->edges.push_back(newEdge);
                        _graphNodes[downNode]->setInputNodeNum(1);
                        _graphNodes[upNum]->setOutputConnection(_graphNodes[downNode]);
                        _currEdge.push_back(newEdge);
                    }
                }
            }
        }
    }
}

// return node position in _graphNodes based off node name and type to account for input/output UiNodes with same names as mx Nodes
int Graph::findNode(std::string name, std::string type)
{
    int count = 0;
    for (size_t i = 0; i < _graphNodes.size(); i++)
    {
        if (_graphNodes[i]->getName() == name)
        {
            if (type == "node" && _graphNodes[i]->getNode() != nullptr)
            {
                return count;
            }
            else if (type == "input" && _graphNodes[i]->getInput() != nullptr)
            {
                return count;
            }
            else if (type == "output" && _graphNodes[i]->getOutput() != nullptr)
            {
                return count;
            }
            else if (type == "nodegraph" && _graphNodes[i]->getNodeGraph() != nullptr)
            {
                return count;
            }
        }
        count++;
    }
    return -1;
}

// set position of pasted nodes based off of original node position
void Graph::positionPasteBin(ImVec2 pos)
{
    ImVec2 totalPos = ImVec2(0, 0);
    ImVec2 avgPos = ImVec2(0, 0);

    // get average position of original nodes
    for (auto pasteNode : _copiedNodes)
    {
        ImVec2 origPos = ed::GetNodePosition(pasteNode.first->getId());
        totalPos.x += origPos.x;
        totalPos.y += origPos.y;
    }
    avgPos.x = totalPos.x / (int) _copiedNodes.size();
    avgPos.y = totalPos.y / (int) _copiedNodes.size();

    // get offset from clciked position
    ImVec2 offset = ImVec2(0, 0);
    offset.x = pos.x - avgPos.x;
    offset.y = pos.y - avgPos.y;
    for (auto pasteNode : _copiedNodes)
    {
        ImVec2 newPos = ImVec2(0, 0);
        newPos.x = ed::GetNodePosition(pasteNode.first->getId()).x + offset.x;
        newPos.y = ed::GetNodePosition(pasteNode.first->getId()).y + offset.y;
        ed::SetNodePosition(pasteNode.second->getId(), newPos);
    }
}
void Graph::createEdge(UiNodePtr upNode, UiNodePtr downNode, mx::InputPtr connectingInput)
{
    if (downNode->getOutput())
    {
        // creating edges for the output nodes
        UiEdge newEdge = UiEdge(upNode, downNode, nullptr);
        if (!edgeExists(newEdge))
        {
            downNode->edges.push_back(newEdge);
            downNode->setInputNodeNum(1);
            upNode->setOutputConnection(downNode);
            _currEdge.push_back(newEdge);
        }
    }
    else if (connectingInput)
    {
        UiEdge newEdge = UiEdge(upNode, downNode, connectingInput);
        downNode->edges.push_back(newEdge);
        downNode->setInputNodeNum(1);
        upNode->setOutputConnection(downNode);
        _currEdge.push_back(newEdge);
    }
}

void Graph::copyUiNode(UiNodePtr node)
{
    UiNodePtr copyNode = std::make_shared<UiNode>(mx::EMPTY_STRING, int(_graphTotalSize + 1));
    ++_graphTotalSize;
    if (node->getMxElement())
    {
        std::string newName = node->getMxElement()->getParent()->createValidChildName(node->getName());
        if (node->getNode())
        {
            mx::NodePtr mxNode;
            mxNode = _currGraphElem->addNodeInstance(node->getNode()->getNodeDef());
            mxNode->copyContentFrom(node->getNode());
            mxNode->setName(newName);
            copyNode->setNode(mxNode);
        }
        else if (node->getInput())
        {
            mx::InputPtr mxInput;
            mxInput = _currGraphElem->addInput(newName);
            mxInput->copyContentFrom(node->getInput());
            copyNode->setInput(mxInput);
        }
        else if (node->getOutput())
        {
            mx::OutputPtr mxOutput;
            mxOutput = _currGraphElem->addOutput(newName);
            mxOutput->copyContentFrom(node->getOutput());
            mxOutput->setName(newName);
            copyNode->setOutput(mxOutput);
        }
        copyNode->getMxElement()->setName(newName);
        copyNode->setName(newName);
    }
    else if (node->getNodeGraph())
    {
        _graphDoc->addNodeGraph();
        std::string nodeGraphName = _graphDoc->getNodeGraphs().back()->getName();
        copyNode->setNodeGraph(_graphDoc->getNodeGraphs().back());
        copyNode->setName(nodeGraphName);
        copyNodeGraph(node, copyNode);
    }
    setUiNodeInfo(copyNode, node->getType(), node->getCategory());
    _copiedNodes[node] = copyNode;
    _graphNodes.push_back(copyNode);
}
void Graph::copyNodeGraph(UiNodePtr origGraph, UiNodePtr copyGraph)
{
    copyGraph->getNodeGraph()->copyContentFrom(origGraph->getNodeGraph());
    std::vector<mx::InputPtr> inputs = copyGraph->getNodeGraph()->getActiveInputs();
    for (mx::InputPtr input : inputs)
    {
        std::string newName = _graphDoc->createValidChildName(input->getName());
        input->setName(newName);
    }
}
void Graph::copyInputs()
{
    for (std::map<UiNodePtr, UiNodePtr>::iterator iter = _copiedNodes.begin(); iter != _copiedNodes.end(); ++iter)
    {
        int count = 0;
        UiNodePtr origNode = iter->first;
        UiNodePtr copyNode = iter->second;
        for (Pin pin : origNode->inputPins)
        {
            if (origNode->getConnectedNode(pin._name) && !_ctrlClick)
            {
                // if original node is connected check if connect node is in copied nodes
                if (_copiedNodes.find(origNode->getConnectedNode(pin._name)) != _copiedNodes.end())
                {
                    // set copy node connected to the value at this key
                    // create an edge
                    createEdge(_copiedNodes[origNode->getConnectedNode(pin._name)], copyNode, copyNode->inputPins[count]._input);
                    UiNodePtr upNode = _copiedNodes[origNode->getConnectedNode(pin._name)];
                    if (copyNode->getNode() || copyNode->getNodeGraph())
                    {

                        mx::InputPtr connectingInput = nullptr;
                        copyNode->inputPins[count]._input->copyContentFrom(pin._input);
                        // update value to be empty
                        if (copyNode->getNode() && copyNode->getNode()->getType() == mx::SURFACE_SHADER_TYPE_STRING)
                        {
                            if (upNode->getOutput())
                            {
                                copyNode->inputPins[count]._input->setConnectedOutput(upNode->getOutput());
                            }
                            else if (upNode->getInput())
                            {

                                copyNode->inputPins[count]._input->setInterfaceName(upNode->getName());
                            }
                            else
                            {
                                // node graph
                                if (upNode->getNodeGraph())
                                {
                                    ed::PinId outputId = getOutputPin(copyNode, upNode, copyNode->inputPins[count]);
                                    for (Pin outPin : upNode->outputPins)
                                    {
                                        if (outPin._pinId == outputId)
                                        {
                                            mx::OutputPtr outputs = upNode->getNodeGraph()->getOutput(outPin._name);
                                            copyNode->inputPins[count]._input->setConnectedOutput(outputs);
                                        }
                                    }
                                }
                                else
                                {
                                    copyNode->inputPins[count]._input->setConnectedNode(upNode->getNode());
                                }
                            }
                        }
                        else
                        {
                            if (upNode->getInput())
                            {

                                copyNode->inputPins[count]._input->setInterfaceName(upNode->getName());
                            }
                            else
                            {
                                copyNode->inputPins[count]._input->setConnectedNode(upNode->getNode());
                            }
                        }

                        copyNode->inputPins[count].setConnected(true);
                        copyNode->inputPins[count]._input->removeAttribute(mx::ValueElement::VALUE_ATTRIBUTE);
                    }
                    else if (copyNode->getOutput() != nullptr)
                    {
                        mx::InputPtr connectingInput = nullptr;
                        copyNode->getOutput()->setConnectedNode(upNode->getNode());
                    }

                    // update input node num and output connections
                    copyNode->setInputNodeNum(1);
                    upNode->setOutputConnection(copyNode);
                }
                else if (pin._input)
                {
                    if (pin._input->getInterfaceInput())
                    {
                        copyNode->inputPins[count]._input->removeAttribute(mx::ValueElement::INTERFACE_NAME_ATTRIBUTE);
                    }
                    copyNode->inputPins[count].setConnected(false);
                    setDefaults(copyNode->inputPins[count]._input);
                    copyNode->inputPins[count]._input->setConnectedNode(nullptr);
                    copyNode->inputPins[count]._input->setConnectedOutput(nullptr);
                }
            }
            count++;
        }
    }
}
// add node to graphNodes based off of node def information
void Graph::addNode(std::string category, std::string name, std::string type)
{
    mx::NodePtr node = nullptr;
    std::vector<mx::NodeDefPtr> matchingNodeDefs;
    // create document or node graph is there is not already one
    if (category == "output")
    {
        std::string outName = "";
        mx::OutputPtr newOut;
        // add output as child of correct parent and create valid name
        outName = _currGraphElem->createValidChildName(name);
        newOut = _currGraphElem->addOutput(outName, type);
        auto outputNode = std::make_shared<UiNode>(outName, int(++_graphTotalSize));
        outputNode->setOutput(newOut);
        setUiNodeInfo(outputNode, type, category);
        return;
    }
    if (category == "input")
    {
        std::string inName = "";
        mx::InputPtr newIn = nullptr;
        // add input as child of correct parent and create valid name
        inName = _currGraphElem->createValidChildName(name);
        newIn = _currGraphElem->addInput(inName, type);
        auto inputNode = std::make_shared<UiNode>(inName, int(++_graphTotalSize));
        setDefaults(newIn);
        inputNode->setInput(newIn);
        setUiNodeInfo(inputNode, type, category);
        return;
    }
    else if (category == "group")
    {
        auto groupNode = std::make_shared<UiNode>(name, int(++_graphTotalSize));
        // set message of group uinode in order to identify it as such
        groupNode->setMessage("Comment");
        setUiNodeInfo(groupNode, type, "group");
        // create ui portions of group node
        buildGroupNode(_graphNodes.back());
        return;
    }
    else if (category == "nodegraph")
    {
        // create new mx::NodeGraph and set as current node graph
        _graphDoc->addNodeGraph();
        std::string nodeGraphName = _graphDoc->getNodeGraphs().back()->getName();
        auto nodeGraphNode = std::make_shared<UiNode>(nodeGraphName, int(++_graphTotalSize));
        // set mx::Nodegraph as node graph for uiNode
        nodeGraphNode->setNodeGraph(_graphDoc->getNodeGraphs().back());

        setUiNodeInfo(nodeGraphNode, type, "nodegraph");
        return;
    }
    // if shader or material we want to add to the document instead of the node graph
    else if (type == mx::SURFACE_SHADER_TYPE_STRING)
    {
        matchingNodeDefs = _graphDoc->getMatchingNodeDefs(category);
        for (mx::NodeDefPtr nodedef : matchingNodeDefs)
        {
            std::string nodedefName = nodedef->getName();
            std::string sub = nodedefName.substr(3, nodedefName.length());
            if (sub == name)
            {
                node = _graphDoc->addNodeInstance(nodedef);
                node->setName(_graphDoc->createValidChildName(name));
                break;
            }
        }
    }
    else if (type == mx::MATERIAL_TYPE_STRING)
    {
        matchingNodeDefs = _graphDoc->getMatchingNodeDefs(category);
        for (mx::NodeDefPtr nodedef : matchingNodeDefs)
        {
            std::string nodedefName = nodedef->getName();
            std::string sub = nodedefName.substr(3, nodedefName.length());
            if (sub == name)
            {
                node = _graphDoc->addNodeInstance(nodedef);
                node->setName(_graphDoc->createValidChildName(name));
                break;
            }
        }
    }
    else
    {
        matchingNodeDefs = _graphDoc->getMatchingNodeDefs(category);
        for (mx::NodeDefPtr nodedef : matchingNodeDefs)
        {
            // use substring of name in order to remove ND_
            std::string nodedefName = nodedef->getName();
            std::string sub = nodedefName.substr(3, nodedefName.length());
            if (sub == name)
            {

                node = _currGraphElem->addNodeInstance(nodedef);
                node->setName(_currGraphElem->createValidChildName(name));
            }
        }
    }
    if (node)
    {
        int num = 0;
        int countDef = 0;
        for (size_t i = 0; i < matchingNodeDefs.size(); i++)
        {
            // use substring of name in order to remove ND_
            std::string nodedefName = matchingNodeDefs[i]->getName();
            std::string sub = nodedefName.substr(3, nodedefName.length());
            if (sub == name)
            {
                num = countDef;
            }
            countDef++;
        }
        std::vector<mx::InputPtr> defInputs = matchingNodeDefs[num]->getActiveInputs();
        // adding inputs to ui node as pins so that we can later add them to the node if necessary
        auto newNode = std::make_shared<UiNode>(node->getName(), int(++_graphTotalSize));
        newNode->setCategory(category);
        newNode->setType(type);
        newNode->setNode(node);
        newNode->_showAllInputs = true;
        node->setType(type);
        ++_graphTotalSize;
        for (mx::InputPtr input : defInputs)
        {
            Pin inPin = Pin(_graphTotalSize, &*input->getName().begin(), input->getType(), newNode, ax::NodeEditor::PinKind::Input, input, nullptr);
            newNode->inputPins.push_back(inPin);
            _currPins.push_back(inPin);
            ++_graphTotalSize;
        }
        std::vector<mx::OutputPtr> defOutputs = matchingNodeDefs[num]->getActiveOutputs();
        for (mx::OutputPtr output : defOutputs)
        {
            Pin outPin = Pin(_graphTotalSize, &*output->getName().begin(), output->getType(), newNode, ax::NodeEditor::PinKind::Output, nullptr, nullptr);
            newNode->outputPins.push_back(outPin);
            _currPins.push_back(outPin);
            ++_graphTotalSize;
        }

        _graphNodes.push_back(std::move(newNode));
        updateMaterials();
    }
}
// return node pos
int Graph::getNodeId(ed::PinId pinId)
{
    for (Pin pin : _currPins)
    {
        if (pin._pinId == pinId)
        {
            return findNode(pin._pinNode->getId());
        }
    }
    return -1;
}

// return pin based off of Pin id
Pin Graph::getPin(ed::PinId pinId)
{
    for (Pin pin : _currPins)
    {
        if (pin._pinId == pinId)
        {
            return pin;
        }
    }
    Pin nullPin = Pin(-10000, "nullPin", "null", nullptr, ax::NodeEditor::PinKind::Output, nullptr, nullptr);
    return nullPin;
}

// This function is based off of the pin icon function in the ImGui Node Editor blueprints-example.cpp
void Graph::DrawPinIcon(std::string type, bool connected, int alpha)
{
    ax::Drawing::IconType iconType = ax::Drawing::IconType::Circle;
    ImColor color = ImColor(0, 0, 0, 255);
    if (_pinColor.find(type) != _pinColor.end())
    {
        color = _pinColor[type];
    }

    color.Value.w = alpha / 255.0f;

    ax::Widgets::Icon(ImVec2(24, 24), iconType, connected, color, ImColor(32, 32, 32, alpha));
}

// This function is based off of the comment node in the ImGui Node Editor blueprints-example.cpp
void Graph::buildGroupNode(UiNodePtr node)
{
    const float commentAlpha = 0.75f;

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha);
    ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(255, 255, 255, 64));
    ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(255, 255, 255, 64));

    ed::BeginNode(node->getId());
    ImGui::PushID(node->getId());

    std::string original = node->getMessage();
    std::string temp = original;
    ImVec2 messageSize = ImGui::CalcTextSize(temp.c_str());
    ImGui::PushItemWidth(messageSize.x + 15);
    ImGui::InputText("##edit", &temp);
    node->setMessage(temp);
    ImGui::PopItemWidth();
    ed::Group(ImVec2(300, 200));
    ImGui::PopID();
    ed::EndNode();
    ed::PopStyleColor(2);
    ImGui::PopStyleVar();
    if (ed::BeginGroupHint(node->getId()))
    {
        auto bgAlpha = static_cast<int>(ImGui::GetStyle().Alpha * 255);
        auto min = ed::GetGroupMin();

        ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
        ImGui::BeginGroup();
        ImGui::PushID(node->getId() + 1000);
        std::string tempName = node->getName();
        ImVec2 nameSize = ImGui::CalcTextSize(temp.c_str());
        ImGui::PushItemWidth(nameSize.x);
        ImGui::InputText("##edit", &tempName);
        node->setName(tempName);
        ImGui::PopID();
        ImGui::EndGroup();

        auto drawList = ed::GetHintBackgroundDrawList();

        ImRect hintBounds = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        ImRect hintFrameBounds = expandImRect(hintBounds, 8, 4);

        drawList->AddRectFilled(
            hintFrameBounds.GetTL(),
            hintFrameBounds.GetBR(),
            IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 4.0f);

        drawList->AddRect(
            hintFrameBounds.GetTL(),
            hintFrameBounds.GetBR(),
            IM_COL32(0, 255, 255, 128 * bgAlpha / 255), 4.0f);
    }
    ed::EndGroupHint();
}
bool Graph::readOnly()
{
    // if the sources are not the same then the current graph cannot be modified
    return _currGraphElem->getActiveSourceUri() != _graphDoc->getActiveSourceUri();
}
mx::InputPtr Graph::findInput(mx::InputPtr nodeInput, std::string name)
{
    if (_isNodeGraph)
    {
        for (UiNodePtr node : _graphNodes)
        {
            if (node->getNode())
            {
                for (mx::InputPtr input : node->getNode()->getActiveInputs())
                {
                    if (input->getInterfaceInput())
                    {

                        if (input->getInterfaceInput() == nodeInput)
                        {
                            return input;
                        }
                    }
                }
            }
        }
    }
    else
    {
        if (_currUiNode->getNodeGraph())
        {
            for (mx::NodePtr node : _currUiNode->getNodeGraph()->getNodes())
            {
                for (mx::InputPtr input : node->getActiveInputs())
                {
                    if (input->getInterfaceInput())
                    {

                        if (input->getInterfaceName() == name)
                        {
                            return input;
                        }
                    }
                }
            }
        }
    }
    return nullptr;
}
//  This function is based off the splitter function in the ImGui Node Editor blueprints-example.cpp
static bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
{
    using namespace ImGui;
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetID("##Splitter");
    ImRect bb;
    bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
    bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
    return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
}

void Graph::outputPin(UiNodePtr node)
{
    // create output pin
    float nodeWidth = 20 + ImGui::CalcTextSize(node->getName().c_str()).x;
    if (nodeWidth < 75)
    {
        nodeWidth = 75;
    }
    const float labelWidth = ImGui::CalcTextSize("output").x;

    // create node editor pin
    for (Pin pin : node->outputPins)
    {
        ImGui::Indent(nodeWidth - labelWidth);
        ed::BeginPin(pin._pinId, ed::PinKind::Output);
        ImGui::Text("%s", pin._name.c_str());
        ImGui::SameLine();
        if (!_pinFilterType.empty())
        {
            if (_pinFilterType == pin._type)
            {
                DrawPinIcon(pin._type, true, DEFAULT_ALPHA);
            }
            else
            {
                DrawPinIcon(pin._type, true, FILTER_ALPHA);
            }
        }
        else
        {
            DrawPinIcon(pin._type, true, DEFAULT_ALPHA);
        }

        ed::EndPin();
        ImGui::Unindent(nodeWidth - labelWidth);
    }
}

void Graph::createInputPin(Pin pin)
{
    ed::BeginPin(pin._pinId, ed::PinKind::Input);
    ImGui::PushID(int(pin._pinId.Get()));
    if (!_pinFilterType.empty())
    {
        if (_pinFilterType == pin._type)
        {
            DrawPinIcon(pin._type, true, DEFAULT_ALPHA);
        }
        else
        {
            DrawPinIcon(pin._type, true, FILTER_ALPHA);
        }
    }
    else
    {
        DrawPinIcon(pin._type, true, DEFAULT_ALPHA);
    }

    ImGui::SameLine();
    ImGui::TextUnformatted(pin._name.c_str());
    ed::EndPin();
    ImGui::PopID();
}

std::vector<int> Graph::createNodes(bool nodegraph)
{
    std::vector<int> outputNum;

    for (UiNodePtr node : _graphNodes)
    {
        if (node->getCategory() == "group")
        {
            buildGroupNode(node);
        }
        else
        {
            // color for output pin
            std::string outputType;
            if (node->getNode() != nullptr)
            {
                ed::BeginNode(node->getId());
                ImGui::PushID(node->getId());
                ImGui::SetWindowFontScale(1.2f);
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetCursorScreenPos() + ImVec2(-7.0, -8.0),
                    ImGui::GetCursorScreenPos() + ImVec2(ed::GetNodeSize(node->getId()).x - 9.f, ImGui::GetTextLineHeight() + 2.f),
                    ImColor(ImColor(55, 55, 55, 255)), 12.f);
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetCursorScreenPos() + ImVec2(-7.0, 3),
                    ImGui::GetCursorScreenPos() + ImVec2(ed::GetNodeSize(node->getId()).x - 9.f, ImGui::GetTextLineHeight() + 2.f),
                    ImColor(ImColor(55, 55, 55, 255)), 0.f);
                ImGui::Text("%s", node->getName().c_str());
                ImGui::SetWindowFontScale(1);

                outputPin(node);
                for (Pin pin : node->inputPins)
                {
                    UiNodePtr upUiNode = node->getConnectedNode(pin._name);
                    if (upUiNode)
                    {
                        size_t pinIndex = 0;
                        if (upUiNode->outputPins.size() > 0)
                        {
                            const std::string outputString = pin._input->getOutputString();
                            if (!outputString.empty())
                            {
                                for (size_t i = 0; i < upUiNode->outputPins.size(); i++)
                                {
                                    Pin& outPin = upUiNode->outputPins[i];
                                    if (outPin._name == outputString)
                                    {
                                        pinIndex = i;
                                        break;
                                    }
                                }
                            }

                            upUiNode->outputPins[pinIndex].addConnection(pin);
                        }
                        pin.setConnected(true);
                    }
                    if (node->_showAllInputs || (pin.getConnected() || node->getNode()->getInput(pin._name)))
                    {
                        createInputPin(pin);
                    }
                }
                // set color of output pin

                if (node->getNode()->getType() == mx::SURFACE_SHADER_TYPE_STRING)
                {
                    if (node->getOutputConnections().size() > 0)
                    {
                        for (UiNodePtr outputCon : node->getOutputConnections())
                        {
                            outputNum.push_back(findNode(outputCon->getId()));
                        }
                    }
                }
            }
            else if (node->getInput() != nullptr)
            {
                ed::BeginNode(node->getId());
                ImGui::PushID(node->getId());
                ImGui::SetWindowFontScale(1.2f);
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetCursorScreenPos() + ImVec2(-7.0f, -8.0f),
                    ImGui::GetCursorScreenPos() + ImVec2(ed::GetNodeSize(node->getId()).x - 9.f, ImGui::GetTextLineHeight() + 2.f),
                    ImColor(ImColor(85, 85, 85, 255)), 12.f);
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetCursorScreenPos() + ImVec2(-7.0f, 3.f),
                    ImGui::GetCursorScreenPos() + ImVec2(ed::GetNodeSize(node->getId()).x - 9.f, ImGui::GetTextLineHeight() + 2.f),
                    ImColor(ImColor(85, 85, 85, 255)), 0.f);
                ImGui::Text("%s", node->getName().c_str());
                ImGui::SetWindowFontScale(1);

                outputType = node->getInput()->getType();
                outputPin(node);
                for (Pin pin : node->inputPins)
                {
                    UiNodePtr upUiNode = node->getConnectedNode(node->getName());
                    if (upUiNode)
                    {
                        if (upUiNode->outputPins.size())
                        {
                            std::string outString = pin._output ? pin._output->getOutputString() : mx::EMPTY_STRING;
                            size_t pinIndex = 0;
                            if (!outString.empty())  
                            {
                                for (size_t i = 0; i<upUiNode->outputPins.size(); i++)
                                {
                                    if (upUiNode->outputPins[i]._name == outString)
                                    {
                                        pinIndex = i;
                                        break;
                                    }
                                }
                            }
                            upUiNode->outputPins[pinIndex].addConnection(pin);
                        }
                        pin.setConnected(true);
                    }
                    ed::BeginPin(pin._pinId, ed::PinKind::Input);
                    if (!_pinFilterType.empty())
                    {
                        if (_pinFilterType == pin._type)
                        {
                            DrawPinIcon(pin._type, true, DEFAULT_ALPHA);
                        }
                        else
                        {
                            DrawPinIcon(pin._type, true, FILTER_ALPHA);
                        }
                    }
                    else
                    {
                        DrawPinIcon(pin._type, true, DEFAULT_ALPHA);
                    }

                    ImGui::SameLine();
                    ImGui::TextUnformatted("value");
                    ed::EndPin();
                }
            }
            else if (node->getOutput() != nullptr)
            {

                ed::BeginNode(node->getId());
                ImGui::PushID(node->getId());
                ImGui::SetWindowFontScale(1.2f);
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetCursorScreenPos() + ImVec2(-7.0, -8.0),
                    ImGui::GetCursorScreenPos() + ImVec2(ed::GetNodeSize(node->getId()).x - 9.f, ImGui::GetTextLineHeight() + 2.f),
                    ImColor(ImColor(35, 35, 35, 255)), 12.f);
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetCursorScreenPos() + ImVec2(-7.0, 3),
                    ImGui::GetCursorScreenPos() + ImVec2(ed::GetNodeSize(node->getId()).x - 9.f, ImGui::GetTextLineHeight() + 2.f),
                    ImColor(ImColor(35, 35, 35, 255)), 0);
                ImGui::Text("%s", node->getName().c_str());
                ImGui::SetWindowFontScale(1.0);

                outputType = node->getOutput()->getType();
                outputPin(node);

                for (Pin pin : node->inputPins)
                {
                    UiNodePtr upUiNode = node->getConnectedNode("");
                    if (upUiNode)
                    {
                        if (upUiNode->outputPins.size())
                        {
                            std::string outString = pin._output ? pin._output->getOutputString() : mx::EMPTY_STRING;
                            size_t pinIndex = 0;
                            if (!outString.empty())  
                            {
                                for (size_t i = 0; i<upUiNode->outputPins.size(); i++)
                                {
                                    if (upUiNode->outputPins[i]._name == outString)
                                    {
                                        pinIndex = i;
                                        break;
                                    }
                                }
                            }
                            upUiNode->outputPins[pinIndex].addConnection(pin);
                        }
                    }

                    ed::BeginPin(pin._pinId, ed::PinKind::Input);
                    if (!_pinFilterType.empty())
                    {
                        if (_pinFilterType == pin._type)
                        {
                            DrawPinIcon(pin._type, true, DEFAULT_ALPHA);
                        }
                        else
                        {
                            DrawPinIcon(pin._type, true, FILTER_ALPHA);
                        }
                    }
                    else
                    {
                        DrawPinIcon(pin._type, true, DEFAULT_ALPHA);
                    }
                    ImGui::SameLine();
                    ImGui::TextUnformatted("input");

                    ed::EndPin();
                }
                if (nodegraph)
                {
                    outputNum.push_back(findNode(node->getId()));
                }
            }
            else if (node->getNodeGraph() != nullptr)
            {
                ed::BeginNode(node->getId());
                ImGui::PushID(node->getId());
                ImGui::SetWindowFontScale(1.2f);
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetCursorScreenPos() + ImVec2(-7.0, -8.0),
                    ImGui::GetCursorScreenPos() + ImVec2(ed::GetNodeSize(node->getId()).x - 9.f, ImGui::GetTextLineHeight() + 2.f),
                    ImColor(ImColor(35, 35, 35, 255)), 12.f);
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetCursorScreenPos() + ImVec2(-7.0, 3),
                    ImGui::GetCursorScreenPos() + ImVec2(ed::GetNodeSize(node->getId()).x - 9.f, ImGui::GetTextLineHeight() + 2.f),
                    ImColor(ImColor(35, 35, 35, 255)), 0);
                ImGui::Text("%s", node->getName().c_str());
                ImGui::SetWindowFontScale(1.0);
                for (Pin pin : node->inputPins)
                {
                    if (node->getConnectedNode(pin._name) != nullptr)
                    {
                        pin.setConnected(true);
                    }
                    if (node->_showAllInputs || (pin.getConnected() || node->getNodeGraph()->getInput(pin._name)))
                    {
                        createInputPin(pin);
                    }
                }
                outputPin(node);
            }
            ImGui::PopID();
            ed::EndNode();
        }
    }
    ImGui::SetWindowFontScale(1.0);
    return outputNum;
}

// add mx::InputPtr to node based off of input pin
void Graph::addNodeInput(UiNodePtr node, mx::InputPtr& input)
{
    if (node->getNode())
    {
        if (!node->getNode()->getInput(input->getName()))
        {
            input = node->getNode()->addInput(input->getName(), input->getType());
            input->setConnectedNode(nullptr);
        }
    }
}
void Graph::setDefaults(mx::InputPtr input)
{
    if (input->getType() == "float")
    {

        input->setValue(0.f, "float");
    }
    else if (input->getType() == "integer")
    {

        input->setValue(0, "integer");
    }
    else if (input->getType() == "color3")
    {

        input->setValue(mx::Color3(0.f, 0.f, 0.f), "color3");
    }
    else if (input->getType() == "color4")
    {
        input->setValue(mx::Color4(0.f, 0.f, 0.f, 1.f), "color4");
    }
    else if (input->getType() == "vector2")
    {
        input->setValue(mx::Vector2(0.f, 0.f), "vector2");
    }
    else if (input->getType() == "vector3")
    {
        input->setValue(mx::Vector3(0.f, 0.f, 0.f), "vector3");
    }
    else if (input->getType() == "vector4")
    {

        input->setValue(mx::Vector4(0.f, 0.f, 0.f, 0.f), "vector4");
    }
    else if (input->getType() == "string")
    {
        input->setValue("", "string");
    }
    else if (input->getType() == "filename")
    {

        input->setValue("", "filename");
    }
    else if (input->getType() == "boolean")
    {

        input->setValue(false, "boolean");
    }
}

// add link to nodegraph and set up connections between UiNodes and MaterialX Nodes to update shader
void Graph::AddLink(ed::PinId inputPinId, ed::PinId outputPinId)
{
    int end_attr = int(outputPinId.Get());
    int start_attr = int(inputPinId.Get());
    Pin inputPin = getPin(outputPinId);
    Pin outputPin = getPin(inputPinId);
    if (inputPinId && outputPinId && (outputPin._type == inputPin._type))
    {
        if (inputPin._connected == false)
        {

            int upNode = getNodeId(inputPinId);
            int downNode = getNodeId(outputPinId);

            // make sure there is an implementation for node
            const mx::ShaderGenerator& shadergen = _renderer->getGenContext().getShaderGenerator();

            // Find the implementation for this nodedef if not an input or output uinode
            if (_graphNodes[downNode]->getInput() && _isNodeGraph)
            {
                ed::RejectNewItem();
                showLabel("Cannot connect to inputs inside of graph", ImColor(50, 50, 50, 255));
                return;
            }
            else if (_graphNodes[upNode]->getNode())
            {
                mx::ShaderNodeImplPtr impl = shadergen.getImplementation(*_graphNodes[upNode]->getNode()->getNodeDef(), _renderer->getGenContext());
                if (!impl)
                {
                    ed::RejectNewItem();
                    showLabel("Invalid Connection: Node does not have an implementation", ImColor(50, 50, 50, 255));
                    return;
                }
            }

            if (ed::AcceptNewItem())
            {
                // Since we accepted new link, lets add one to our list of links.
                Link link;
                link._startAttr = start_attr;
                link._endAttr = end_attr;
                _currLinks.push_back(link);
                _frameCount = ImGui::GetFrameCount();
                _renderer->setMaterialCompilation(true);

                if (_graphNodes[downNode]->getNode() || _graphNodes[downNode]->getNodeGraph())
                {

                    mx::InputPtr connectingInput = nullptr;
                    for (Pin& pin : _graphNodes[downNode]->inputPins)
                    {
                        if (pin._pinId == outputPinId)
                        {
                            addNodeInput(_graphNodes[downNode], pin._input);
                            // update value to be empty
                            if (_graphNodes[downNode]->getNode() && _graphNodes[downNode]->getNode()->getType() == mx::SURFACE_SHADER_TYPE_STRING)
                            {
                                if (_graphNodes[upNode]->getOutput() != nullptr)
                                {
                                    pin._input->setConnectedOutput(_graphNodes[upNode]->getOutput());
                                }
                                else
                                {
                                    // node graph
                                    if (_graphNodes[upNode]->getNodeGraph() != nullptr)
                                    {
                                        for (Pin outPin : _graphNodes[upNode]->outputPins)
                                        {
                                            // set pin connection to correct output
                                            if (outPin._pinId == inputPinId)
                                            {
                                                mx::OutputPtr outputs = _graphNodes[upNode]->getNodeGraph()->getOutput(outPin._name);
                                                pin._input->setConnectedOutput(outputs);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        pin._input->setConnectedNode(_graphNodes[upNode]->getNode());
                                    }
                                }
                            }
                            else
                            {
                                if (_graphNodes[upNode]->getInput())
                                {

                                    pin._input->setInterfaceName(_graphNodes[upNode]->getName());
                                }
                                else
                                {
                                    if (_graphNodes[upNode]->getNode())
                                    {
                                        pin._input->setConnectedNode(_graphNodes[upNode]->getNode());
                                    }
                                    else if (_graphNodes[upNode]->getNodeGraph())
                                    {
                                        for (Pin outPin : _graphNodes[upNode]->outputPins)
                                        {
                                            // set pin connection to correct output
                                            if (outPin._pinId == inputPinId)
                                            {
                                                mx::OutputPtr outputs = _graphNodes[upNode]->getNodeGraph()->getOutput(outPin._name);
                                                pin._input->setConnectedOutput(outputs);
                                            }
                                        }
                                    }
                                }
                            }

                            pin.setConnected(true);
                            pin._input->removeAttribute(mx::ValueElement::VALUE_ATTRIBUTE);
                            connectingInput = pin._input;
                            break;
                        }
                    }
                    // create new edge and set edge information
                    createEdge(_graphNodes[upNode], _graphNodes[downNode], connectingInput);
                }
                else if (_graphNodes[downNode]->getOutput() != nullptr)
                {
                    mx::InputPtr connectingInput = nullptr;
                    _graphNodes[downNode]->getOutput()->setConnectedNode(_graphNodes[upNode]->getNode());

                    // create new edge and set edge information
                    createEdge(_graphNodes[upNode], _graphNodes[downNode], connectingInput);
                }
                else
                {
                    // create new edge and set edge info
                    UiEdge newEdge = UiEdge(_graphNodes[upNode], _graphNodes[downNode], nullptr);
                    if (!edgeExists(newEdge))
                    {
                        _graphNodes[downNode]->edges.push_back(newEdge);
                        _currEdge.push_back(newEdge);

                        // update input node num and output connections
                        _graphNodes[downNode]->setInputNodeNum(1);
                        _graphNodes[upNode]->setOutputConnection(_graphNodes[downNode]);
                    }
                }
            }
        }
        else
        {
            ed::RejectNewItem();
        }
    }
    else
    {
        ed::RejectNewItem();
        showLabel("Invalid Connection due to Mismatch Types", ImColor(50, 50, 50, 255));
    }
}

void Graph::deleteLinkInfo(int startAttr, int endAttr)
{
    int upNode = getNodeId(startAttr);
    int downNode = getNodeId(endAttr);
    int num = _graphNodes[downNode]->getEdgeIndex(_graphNodes[upNode]->getId());
    if (num != -1)
    {
        if (_graphNodes[downNode]->edges.size() == 1)
        {
            _graphNodes[downNode]->edges.erase(_graphNodes[downNode]->edges.begin() + 0);
        }
        else if (_graphNodes[downNode]->edges.size() > 1)
        {
            _graphNodes[downNode]->edges.erase(_graphNodes[downNode]->edges.begin() + num);
        }
    }

    // downNode set node num -1
    _graphNodes[downNode]->setInputNodeNum(-1);
    // upNode remove outputconnection
    _graphNodes[upNode]->removeOutputConnection(_graphNodes[downNode]->getName());
    // change input so that is default val
    // change informtion of actual mx::Node
    if (_graphNodes[downNode]->getNode())
    {
        mx::NodeDefPtr nodeDef = _graphNodes[downNode]->getNode()->getNodeDef(_graphNodes[downNode]->getNode()->getName());

        for (Pin& pin : _graphNodes[downNode]->inputPins)
        {

            if ((int) pin._pinId.Get() == endAttr)
            {

                mx::ValuePtr val = nodeDef->getActiveInput(pin._input->getName())->getValue();
                if (_graphNodes[downNode]->getNode()->getType() == mx::SURFACE_SHADER_TYPE_STRING && _graphNodes[upNode]->getNodeGraph())
                {
                    pin._input->setConnectedOutput(nullptr);
                }
                else
                {
                    pin._input->setConnectedNode(nullptr);
                }
                if (_graphNodes[upNode]->getInput())
                {
                    // remove interface value in order to set the default of the input
                    pin._input->removeAttribute(mx::ValueElement::INTERFACE_NAME_ATTRIBUTE);
                    setDefaults(pin._input);
                    setDefaults(_graphNodes[upNode]->getInput());
                }

                pin.setConnected(false);
                // if a value exists update the input with it
                if (val)
                {
                    pin._input->setValueString(val->getValueString());
                }
            }
        }
    }
    else if (_graphNodes[downNode]->getNodeGraph())
    {
        // set default values for nodegraph node pins ie nodegraph inputs
        mx::NodeDefPtr nodeDef = _graphNodes[downNode]->getNodeGraph()->getNodeDef();
        for (Pin pin : _graphNodes[downNode]->inputPins)
        {

            if ((int) pin._pinId.Get() == endAttr)
            {

                if (_graphNodes[upNode]->getInput())
                {
                    _graphNodes[downNode]->getNodeGraph()->getInput(pin._name)->removeAttribute(mx::ValueElement::INTERFACE_NAME_ATTRIBUTE);
                    setDefaults(_graphNodes[upNode]->getInput());
                }
                pin._input->setConnectedNode(nullptr);
                pin.setConnected(false);
                setDefaults(pin._input);
            }
        }
    }
    else if (_graphNodes[downNode]->getOutput())
    {
        for (Pin pin : _graphNodes[downNode]->inputPins)
        {
            _graphNodes[downNode]->getOutput()->removeAttribute("nodename");
            pin.setConnected(false);
        }
    }
}
// delete link from currLink vector and remove any connections in UiNode or MaterialX Nodes to update shader
void Graph::deleteLink(ed::LinkId deletedLinkId)
{
    // If you agree that link can be deleted, accept deletion.
    if (ed::AcceptDeletedItem())
    {
        _renderer->setMaterialCompilation(true);
        _frameCount = ImGui::GetFrameCount();
        int link_id = int(deletedLinkId.Get());
        // Then remove link from your data.
        int pos = findLinkPosition(link_id);

        // link start -1 equals node num
        Link currLink = _currLinks[pos];
        deleteLinkInfo(currLink._startAttr, currLink._endAttr);
        _currLinks.erase(_currLinks.begin() + pos);
    }
}

void Graph::deleteNode(UiNodePtr node)
{
    // delete link
    for (Pin inputPins : node->inputPins)
    {
        UiNodePtr upNode = node->getConnectedNode(inputPins._name);
        if (upNode)
        {
            upNode->removeOutputConnection(node->getName());
            int num = node->getEdgeIndex(upNode->getId());
            // erase edge between node and up node
            if (num != -1)
            {
                if (node->edges.size() == 1)
                {
                    node->edges.erase(node->edges.begin() + 0);
                }
                else if (node->edges.size() > 1)
                {
                    node->edges.erase(node->edges.begin() + num);
                }
            }
        }
    }
    // update downNode info
    std::vector<Pin> outputConnections = node->outputPins.front().getConnections();

    for (Pin pin : outputConnections)
    {
        mx::ValuePtr val;
        if (pin._pinNode->getNode())
        {
            mx::NodeDefPtr nodeDef = pin._pinNode->getNode()->getNodeDef(pin._pinNode->getNode()->getName());
            val = nodeDef->getActiveInput(pin._input->getName())->getValue();
            if (pin._pinNode->getNode()->getType() == "surfaceshader")
            {
                pin._input->setConnectedOutput(nullptr);
            }
            else
            {
                pin._input->setConnectedNode(nullptr);
            }
        }
        else if (pin._pinNode->getNodeGraph())
        {
            if (node->getInput())
            {
                pin._pinNode->getNodeGraph()->getInput(pin._name)->removeAttribute(mx::ValueElement::INTERFACE_NAME_ATTRIBUTE);
                setDefaults(node->getInput());
            }
            pin._input->setConnectedNode(nullptr);
            pin.setConnected(false);
            setDefaults(pin._input);
        }

        pin.setConnected(false);
        if (val)
        {
            pin._input->setValueString(val->getValueString());
        }

        int num = pin._pinNode->getEdgeIndex(node->getId());
        if (num != -1)
        {
            if (pin._pinNode->edges.size() == 1)
            {
                pin._pinNode->edges.erase(pin._pinNode->edges.begin() + 0);
            }
            else if (pin._pinNode->edges.size() > 1)
            {
                pin._pinNode->edges.erase(pin._pinNode->edges.begin() + num);
            }
        }

        pin._pinNode->setInputNodeNum(-1);
        // not really necessary since it will be deleted
        node->removeOutputConnection(pin._pinNode->getName());
    }

    // remove from NodeGraph
    // all link information is handled in delete link which is called before this
    int nodeNum = findNode(node->getId());
    _currGraphElem->removeChild(node->getName());
    _graphNodes.erase(_graphNodes.begin() + nodeNum);
}

// create pins for outputs/inputs added while inside the node graph
void Graph::addNodeGraphPins()
{
    for (UiNodePtr node : _graphNodes)
    {
        if (node->getNodeGraph())
        {
            if (node->inputPins.size() != node->getNodeGraph()->getInputs().size())
            {
                for (mx::InputPtr input : node->getNodeGraph()->getInputs())
                {
                    std::string name = input->getName();
                    auto result = std::find_if(node->inputPins.begin(), node->inputPins.end(), [name](const Pin& x)
                                               {
                        return x._name == name;
                    });
                    if (result == node->inputPins.end())
                    {
                        Pin inPin = Pin(++_graphTotalSize, &*input->getName().begin(), input->getType(), node, ax::NodeEditor::PinKind::Input, input, nullptr);
                        node->inputPins.push_back(inPin);
                        _currPins.push_back(inPin);
                        ++_graphTotalSize;
                    }
                }
            }
            if (node->outputPins.size() != node->getNodeGraph()->getOutputs().size())
            {
                for (mx::OutputPtr output : node->getNodeGraph()->getOutputs())
                {
                    std::string name = output->getName();
                    auto result = std::find_if(node->outputPins.begin(), node->outputPins.end(), [name](const Pin& x)
                                               {
                        return x._name == name;
                    });
                    if (result == node->outputPins.end())
                    {
                        Pin outPin = Pin(++_graphTotalSize, &*output->getName().begin(), output->getType(), node, ax::NodeEditor::PinKind::Output, nullptr, nullptr);
                        ++_graphTotalSize;
                        node->outputPins.push_back(outPin);
                        _currPins.push_back(outPin);
                    }
                }
            }
        }
    }
}

void Graph::upNodeGraph()
{
    if (!_graphStack.empty())
    {
        savePosition();
        _graphNodes = _graphStack.top();
        _currPins = _pinStack.top();
        _graphTotalSize = _sizeStack.top();
        addNodeGraphPins();
        _graphStack.pop();
        _pinStack.pop();
        _sizeStack.pop();
        _currGraphName.pop_back();
        _initial = true;
        ed::NavigateToContent();
        if (_currUiNode)
        {
            ed::DeselectNode(_currUiNode->getId());
            _currUiNode = nullptr;
        }
        _prevUiNode = nullptr;
        _isNodeGraph = false;
        _currGraphElem = _graphDoc;
        _initial = true;
    }
}

void Graph::graphButtons()
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.15f, .15f, .15f, 1.0f));

    // buttons for loading and saving a .mtlx
    // new Material button
    if (ImGui::Button("New Material"))
    {
        _graphNodes.clear();
        _currLinks.clear();
        _currEdge.clear();
        _newLinks.clear();
        _currPins.clear();
        _graphDoc = mx::createDocument();
        _graphDoc->importLibrary(_stdLib);
        _currGraphElem = _graphDoc;

        if (_currUiNode != nullptr)
        {
            ed::DeselectNode(_currUiNode->getId());
            _currUiNode = nullptr;
        }
        _prevUiNode = nullptr;
        _currRenderNode = nullptr;
        _isNodeGraph = false;
        _currGraphName.clear();

        _renderer->setDocument(_graphDoc);
        _renderer->updateMaterials(nullptr);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Material"))
    {
        // deselect node before loading new file
        if (_currUiNode != nullptr)
        {
            ed::DeselectNode(_currUiNode->getId());
            _currUiNode = nullptr;
        }

        _fileDialog.SetTitle("Open File Window");
        _fileDialog.Open();
    }
    ImGui::SameLine();
    if (ImGui::Button("Save Material"))
    {
        _fileDialogSave.SetTitle("Save File Window");
        _fileDialogSave.Open();
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto Layout"))
    {
        _autoLayout = true;
    }

    // split window into panes for NodeEditor
    static float leftPaneWidth = 375.0f;
    static float rightPaneWidth = 750.0f;
    Splitter(true, 4.0f, &leftPaneWidth, &rightPaneWidth, 20.0f, 20.0f);
    // create back button and graph hiearchy name display
    ImGui::Indent(leftPaneWidth + 15.f);
    if (ImGui::Button("<"))
    {
        upNodeGraph();
    }
    ImGui::SameLine();
    if (!_currGraphName.empty())
    {
        for (std::string name : _currGraphName)
        {
            ImGui::Text("%s", name.c_str());
            ImGui::SameLine();
            if (name != _currGraphName.back())
            {
                ImGui::Text(">");
                ImGui::SameLine();
            }
        }
    }
    ImVec2 windowPos2 = ImGui::GetWindowPos();
    ImGui::Unindent(leftPaneWidth + 15.f);
    ImGui::PopStyleColor();
    ImGui::NewLine();
    // creating two windows using splitter
    float paneWidth = (leftPaneWidth - 2.0f);
    ImGui::BeginChild("Selection", ImVec2(paneWidth, 0));
    ImVec2 windowPos = ImGui::GetWindowPos();
    // renderView window
    ImVec2 wsize = ImVec2((float) _renderer->_screenWidth, (float) _renderer->_screenHeight);
    float aspectRatio = _renderer->_pixelRatio;
    ImVec2 screenSize = ImVec2(paneWidth, paneWidth / aspectRatio);
    _renderer->_screenWidth = (unsigned int) screenSize[0];
    _renderer->_screenHeight = (unsigned int) screenSize[1];

    if (_renderer != nullptr)
    {

        glEnable(GL_FRAMEBUFFER_SRGB);
        _renderer->getViewCamera()->setViewportSize(mx::Vector2(screenSize[0], screenSize[1]));
        GLuint64 my_image_texture = _renderer->_textureID;
        mx::Vector2 vec = _renderer->getViewCamera()->getViewportSize();
        // current image has correct color space but causes problems for gui
        ImGui::Image((ImTextureID) my_image_texture, screenSize, ImVec2(0, 1), ImVec2(1, 0));
    }
    ImGui::Separator();

    // property editor for current nodes
    propertyEditor();
    ImGui::EndChild();
    ImGui::SameLine(0.0f, 12.0f);

    handleRenderViewInputs(windowPos, screenSize[0], screenSize[1]);
}
void Graph::propertyEditor()
{
    ImGui::Text("Node Property Editor");
    if (_currUiNode)
    {
        // set and edit name
        ImGui::Text("Name: ");
        ImGui::SameLine();
        std::string original = _currUiNode->getName();
        std::string temp = original;
        ImGui::PushItemWidth(100.0f);
        ImGui::InputText("##edit", &temp);
        ImGui::PopItemWidth();
        std::string docString = "NodeDef Doc String: \n";
        if (_currUiNode->getNode())
        {
            if (temp != original)
            {

                std::string name = _currUiNode->getNode()->getParent()->createValidChildName(temp);

                std::vector<UiNodePtr> downstreamNodes = _currUiNode->getOutputConnections();
                for (UiNodePtr nodes : downstreamNodes)
                {
                    if (nodes->getInput() == nullptr)
                    {
                        for (mx::InputPtr input : nodes->getNode()->getActiveInputs())
                        {
                            if (input->getConnectedNode() == _currUiNode->getNode())
                            {
                                _currUiNode->getNode()->setName(name);
                                nodes->getNode()->setConnectedNode(input->getName(), _currUiNode->getNode());
                            }
                        }
                    }
                }
                _currUiNode->setName(name);
                _currUiNode->getNode()->setName(name);
            }
        }
        else if (_currUiNode->getInput())
        {
            if (temp != original)
            {

                std::string name = _currUiNode->getInput()->getParent()->createValidChildName(temp);

                std::vector<UiNodePtr> downstreamNodes = _currUiNode->getOutputConnections();
                for (UiNodePtr nodes : downstreamNodes)
                {
                    if (nodes->getInput() == nullptr)
                    {
                        if (nodes->getNode())
                        {
                            for (mx::InputPtr input : nodes->getNode()->getActiveInputs())
                            {
                                if (input->getInterfaceInput() == _currUiNode->getInput())
                                {
                                    _currUiNode->getInput()->setName(name);
                                    mx::ValuePtr val = _currUiNode->getInput()->getValue();
                                    input->setInterfaceName(name);
                                    mx::InputPtr pt = input->getInterfaceInput();
                                }
                            }
                        }
                        else
                        {
                            nodes->getOutput()->setConnectedNode(_currUiNode->getNode());
                        }
                    }
                }

                _currUiNode->getInput()->setName(name);
                _currUiNode->setName(name);
            }
        }
        else if (_currUiNode->getOutput())
        {
            if (temp != original)
            {
                std::string name = _currUiNode->getOutput()->getParent()->createValidChildName(temp);
                _currUiNode->getOutput()->setName(name);
                _currUiNode->setName(name);
            }
        }
        else if (_currUiNode->getCategory() == "group")
        {
            _currUiNode->setName(temp);
        }
        else if (_currUiNode->getCategory() == "nodegraph")
        {
            if (temp != original)
            {
                std::string name = _currUiNode->getNodeGraph()->getParent()->createValidChildName(temp);
                _currUiNode->getNodeGraph()->setName(name);
                _currUiNode->setName(name);
            }
        }

        ImGui::Text("Category:");
        ImGui::SameLine();
        // change button color to match background
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.096f, .096f, .096f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.1f, .1f, .1f, 1.0f));
        if (_currUiNode->getNode())
        {
            ImGui::Text("%s", _currUiNode->getNode()->getCategory().c_str());
            docString += _currUiNode->getNode()->getCategory().c_str();
            docString += ":";
            docString += _currUiNode->getNode()->getNodeDef()->getDocString() + "\n";
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::SetTooltip("%s", _currUiNode->getNode()->getNodeDef()->getDocString().c_str());
            }

            ImGui::Text("Inputs:");
            ImGui::Indent();

            for (Pin& input : _currUiNode->inputPins)
            {
                if (_currUiNode->_showAllInputs || (input.getConnected() || _currUiNode->getNode()->getInput(input._name)))
                {
                    mx::OutputPtr out = input._input->getConnectedOutput();
                    // setting comment help box
                    ImGui::PushID(int(input._pinId.Get()));
                    ImGui::Text("%s", input._input->getName().c_str());
                    mx::InputPtr tempInt = _currUiNode->getNode()->getNodeDef()->getActiveInput(input._input->getName());
                    docString += input._name;
                    docString += ": ";
                    if (tempInt)
                    {
                        std::string newStr = _currUiNode->getNode()->getNodeDef()->getActiveInput(input._input->getName())->getDocString();
                        if (newStr != mx::EMPTY_STRING)
                        {
                            docString += newStr;
                        }
                    }
                    docString += "\t \n";
                    ImGui::SameLine();
                    std::string typeText = " [" + input._input->getType() + "]";
                    ImGui::Text("%s", typeText.c_str());

                    // setting constant sliders for input values
                    if (!input.getConnected())
                    {
                        setConstant(_currUiNode, input._input);
                    }

                    ImGui::PopID();
                }
            }

            ImGui::Unindent();
            ImGui::Checkbox("Show all inputs", &_currUiNode->_showAllInputs);
        }

        else if (_currUiNode->getInput() != nullptr)
        {
            ImGui::Text("%s", _currUiNode->getCategory().c_str());
            std::vector<Pin> inputs = _currUiNode->inputPins;
            ImGui::Text("Inputs:");
            ImGui::Indent();
            for (size_t i = 0; i < inputs.size(); i++)
            {

                // setting comment help box
                ImGui::PushID(int(inputs[i]._pinId.Get()));
                ImGui::Text("%s", inputs[i]._input->getName().c_str());

                ImGui::SameLine();
                std::string typeText = " [" + inputs[i]._input->getType() + "]";
                ImGui::Text("%s", typeText.c_str());
                // setting constant sliders for input values
                if (!inputs[i].getConnected())
                {
                    setConstant(_currUiNode, inputs[i]._input);
                }
                ImGui::PopID();
            }
            ImGui::Unindent();
        }
        else if (_currUiNode->getOutput() != nullptr)
        {
            ImGui::Text("%s", _currUiNode->getOutput()->getCategory().c_str());
        }
        else if (_currUiNode->getNodeGraph() != nullptr)
        {
            std::vector<Pin> inputs = _currUiNode->inputPins;
            ImGui::Text("%s", _currUiNode->getCategory().c_str());
            ImGui::Text("Inputs:");
            ImGui::Indent();
            int count = 0;
            for (Pin input : inputs)
            {
                if (_currUiNode->_showAllInputs || (input.getConnected() || _currUiNode->getNodeGraph()->getInput(input._name)))
                {
                    // setting comment help box
                    ImGui::PushID(int(input._pinId.Get()));
                    ImGui::Text("%s", input._input->getName().c_str());

                    docString += _currUiNode->getNodeGraph()->getActiveInput(input._input->getName())->getDocString();

                    ImGui::SameLine();
                    std::string typeText = " [" + input._input->getType() + "]";
                    ImGui::Text("%s", typeText.c_str());
                    if (!input._input->getConnectedNode() && _currUiNode->getNodeGraph()->getActiveInput(input._input->getName()))
                    {
                        setConstant(_currUiNode, input._input);
                    }

                    ImGui::PopID();
                    count++;
                }
            }
            ImGui::Unindent();
            ImGui::Checkbox("Show all inputs", &_currUiNode->_showAllInputs);
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();

        if (ImGui::Button("Node Info"))
        {
            ImGui::OpenPopup("docstring");
        }

        if (ImGui::BeginPopup("docstring"))
        {
            ImGui::Text("%s", docString.c_str());
            ImGui::EndPopup();
        }
    }
}
void Graph::addNodePopup(bool cursor)
{
    bool open_AddPopup = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyReleased(GLFW_KEY_TAB);
    if (open_AddPopup)
    {
        cursor = true;
        ImGui::OpenPopup("add node");
    }
    if (ImGui::BeginPopup("add node"))
    {
        ImGui::Text("Add Node");
        ImGui::Separator();
        static char input[16]{ "" };
        if (cursor)
        {
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::InputText("##input", input, sizeof(input));
        std::string subs(input);
        // input string length
        // filter extra nodes - includes inputs, outputs, groups, and node graphs
        for (std::unordered_map<std::string, std::vector<std::vector<std::string>>>::iterator it = _extraNodes.begin(); it != _extraNodes.end(); ++it)
        {
            // filter out list of nodes
            if (subs.size() > 0)
            {
                for (size_t i = 0; i < it->second.size(); i++)
                {
                    std::string str(it->second[i][0]);
                    std::string nodeName = it->second[i][0];
                    if (str.find(subs) != std::string::npos)
                    {
                        if (ImGui::MenuItem(nodeName.substr(3, nodeName.length()).c_str()) || (ImGui::IsItemFocused() && ImGui::IsKeyPressedMap(ImGuiKey_Enter)))
                        {
                            addNode(it->second[i][2], nodeName.substr(3, nodeName.length()), it->second[i][1]);
                            _addNewNode = true;
                            memset(input, '\0', sizeof(input));
                        }
                    }
                }
            }
            else
            {
                ImGui::SetNextWindowSizeConstraints(ImVec2(100, 10), ImVec2(250, 300));
                if (ImGui::BeginMenu(it->first.c_str()))
                {
                    for (size_t j = 0; j < it->second.size(); j++)
                    {
                        std::string name = it->second[j][0];
                        if (ImGui::MenuItem(name.substr(3, name.length()).c_str()) || (ImGui::IsItemFocused() && ImGui::IsKeyPressedMap(ImGuiKey_Enter)))
                        {
                            addNode(it->second[j][2], name.substr(3, name.length()), it->second[j][1]);
                            _addNewNode = true;
                        }
                    }
                    ImGui::EndMenu();
                }
            }
        }
        // filter nodedefs and add to menu if matches filter
        for (std::unordered_map<std::string, std::vector<mx::NodeDefPtr>>::iterator it = _nodesToAdd.begin(); it != _nodesToAdd.end(); ++it)
        {
            // filter out list of nodes
            if (subs.size() > 0)
            {
                for (size_t i = 0; i < it->second.size(); i++)
                {
                    std::string str(it->second[i]->getName());
                    std::string nodeName = it->second[i]->getName();
                    if (str.find(subs) != std::string::npos)
                    {
                        if (ImGui::MenuItem(it->second[i]->getName().substr(3, nodeName.length()).c_str()) || (ImGui::IsItemFocused() && ImGui::IsKeyPressedMap(ImGuiKey_Enter)))
                        {
                            addNode(it->second[i]->getNodeString(), it->second[i]->getName().substr(3, nodeName.length()), it->second[i]->getType());
                            _addNewNode = true;
                            memset(input, '\0', sizeof(input));
                        }
                    }
                }
            }
            else
            {
                ImGui::SetNextWindowSizeConstraints(ImVec2(100, 10), ImVec2(250, 300));
                if (ImGui::BeginMenu(it->first.c_str()))
                {
                    for (size_t i = 0; i < it->second.size(); i++)
                    {

                        std::string name = it->second[i]->getName();
                        if (ImGui::MenuItem(it->second[i]->getName().substr(3, name.length()).c_str()) || (ImGui::IsItemFocused() && ImGui::IsKeyPressedMap(ImGuiKey_Enter)))
                        {
                            addNode(it->second[i]->getNodeString(), it->second[i]->getName().substr(3, name.length()), it->second[i]->getType());
                            _addNewNode = true;
                        }
                    }
                    ImGui::EndMenu();
                }
            }
        }
        cursor = false;
        ImGui::EndPopup();
        open_AddPopup = false;
    }
}
void Graph::searchNodePopup(bool cursor)
{
    const bool open_search = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyDown(GLFW_KEY_F) && ImGui::IsKeyDown(GLFW_KEY_LEFT_CONTROL);
    if (open_search)
    {
        cursor = true;
        ImGui::OpenPopup("search");
    }
    if (ImGui::BeginPopup("search"))
    {
        ed::NavigateToSelection();
        static ImGuiTextFilter filter;
        ImGui::Text("Search for Node:");
        static char input[16]{ "" };
        ImGui::SameLine();
        if (cursor)
        {
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::InputText("##input", input, sizeof(input));

        if (std::string(input).size() > 0)
        {

            for (UiNodePtr node : _graphNodes)
            {
                if (node->getName().find(std::string(input)) != std::string::npos)
                {

                    if (ImGui::MenuItem(node->getName().c_str()) || (ImGui::IsItemFocused() && ImGui::IsKeyPressedMap(ImGuiKey_Enter)))
                    {
                        _searchNodeId = node->getId();
                        memset(input, '\0', sizeof(input));
                    }
                }
            }
        }
        cursor = false;
        ImGui::EndPopup();
    }
}

void Graph::readOnlyPopup()
{
    if (_popup)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 100));
        ImGui::OpenPopup("Read Only");
        _popup = false;
    }
    if (ImGui::BeginPopup("Read Only"))
    {
        ImGui::Text("This graph is Read Only");
        ImGui::EndPopup();
    }
}

// compiling shaders message
void Graph::shaderPopup()
{
    if (_renderer->getMaterialCompilation())
    {
        ImGui::SetNextWindowPos(ImVec2((float) _renderer->_screenWidth - 135, (float) _renderer->_screenHeight + 5));
        ImGui::SetNextWindowBgAlpha(80.f);
        ImGui::OpenPopup("Shaders");
    }
    if (ImGui::BeginPopup("Shaders"))
    {
        ImGui::Text("Compiling Shaders");
        if (!_renderer->getMaterialCompilation())
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// allow for camera manipulation of render view window
void Graph::handleRenderViewInputs(ImVec2 minValue, float width, float height)
{
    ImVec2 mousePos = ImGui::GetMousePos();
    mx::Vector2 mxMousePos = mx::Vector2(mousePos.x, mousePos.y);
    ImVec2 dragDelta = ImGui::GetMouseDragDelta();
    float scrollAmt = ImGui::GetIO().MouseWheel;
    int button = -1;
    bool down = false;
    if (mousePos.x > minValue.x && mousePos.x < (minValue.x + width) && mousePos.y > minValue.y && mousePos.y < (minValue.y + height))
    {
        if (ImGui::IsMouseDragging(0) || ImGui::IsMouseDragging(1))
        {
            _renderer->setMouseMotionEvent(mxMousePos);
        }
        if (ImGui::IsMouseClicked(0))
        {
            button = 0;
            down = true;
            _renderer->setMouseButtonEvent(button, down, mxMousePos);
        }
        else if (ImGui::IsMouseClicked(1))
        {
            button = 1;
            down = true;
            _renderer->setMouseButtonEvent(button, down, mxMousePos);
        }
        else if (ImGui::IsMouseReleased(0))
        {
            button = 0;
            _renderer->setMouseButtonEvent(button, down, mxMousePos);
        }
        else if (ImGui::IsMouseReleased(1))
        {
            button = 1;
            _renderer->setMouseButtonEvent(button, down, mxMousePos);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_KeypadAdd))
        {
            _renderer->setKeyEvent(ImGuiKey_KeypadAdd);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract))
        {
            _renderer->setKeyEvent(ImGuiKey_KeypadSubtract);
        }
        // scrolling not possible if open or save file dialog is open
        if (scrollAmt != 0 && !_fileDialogSave.IsOpened() && !_fileDialog.IsOpened())
        {
            _renderer->setScrollEvent(scrollAmt);
        }
    }
}
// sets up graph editor
void Graph::drawGraph(ImVec2 mousePos)
{

    if (_searchNodeId > 0)
    {
        ed::SelectNode(_searchNodeId);
        ed::NavigateToSelection();
        _searchNodeId = -1;
    }

    bool TextCursor = false;
    // center imgui window and setting size
    ImGuiIO& io2 = ImGui::GetIO();
    ImGui::SetNextWindowSize(io2.DisplaySize);
    ImGui::SetNextWindowPos(ImVec2(io2.DisplaySize.x * 0.5f, io2.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("MaterialX", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);

    io2.ConfigFlags = ImGuiConfigFlags_IsSRGB | ImGuiConfigFlags_NavEnableKeyboard;
    io2.MouseDoubleClickTime = .5;
    // increase default font size
    ImFont* f = ImGui::GetFont();
    f->FontSize = 14;

    graphButtons();

    ed::Begin("My Editor");
    {
        ed::Suspend();
        // set up pop ups for adding a node when tab is pressed
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
        ImGui::SetNextWindowSize({ 250.0f, 300.0f });
        addNodePopup(TextCursor);
        searchNodePopup(TextCursor);
        readOnlyPopup();
        ImGui::PopStyleVar();

        ed::Resume();

        // Gathering selected nodes / links - from ImGui Node Editor blueprints-example.cpp
        std::vector<ed::NodeId> selectedNodes;
        std::vector<ed::LinkId> selectedLinks;
        selectedNodes.resize(ed::GetSelectedObjectCount());
        selectedLinks.resize(ed::GetSelectedObjectCount());

        int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
        int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

        selectedNodes.resize(nodeCount);
        selectedLinks.resize(linkCount);
        if (io2.KeyCtrl && io2.MouseDown[0])
        {
            _ctrlClick = true;
        }

        // setting current node based off of selected node
        if (selectedNodes.size() > 0)
        {
            int graphPos = findNode(int(selectedNodes[0].Get()));
            if (graphPos > -1)
            {
                // only selected not if its not the same as previously selected
                if (!_prevUiNode || (_prevUiNode->getName() != _graphNodes[graphPos]->getName()))
                {
                    _currUiNode = _graphNodes[graphPos];
                    // update render material if needed
                    if (_currUiNode->getNode())
                    {
                        if (_currUiNode->getNode()->getType() == mx::SURFACE_SHADER_TYPE_STRING || _currUiNode->getNode()->getType() == mx::MATERIAL_TYPE_STRING)
                        {
                            setRenderMaterial(_currUiNode);
                        }
                    }
                    else if (_currUiNode->getNodeGraph())
                    {
                        setRenderMaterial(_currUiNode);
                    }
                    else if (_currUiNode->getOutput())
                    {
                        setRenderMaterial(_currUiNode);
                    }
                    _prevUiNode = _currUiNode;
                }
            }
        }

        // check if keyboard shortcuts for copy/cut/paste have been used
        if (ed::BeginShortcut())
        {
            if (ed::AcceptCopy())
            {
                _copiedNodes.clear();
                for (ed::NodeId selected : selectedNodes)
                {
                    int pos = findNode((int) selected.Get());
                    if (pos >= 0)
                    {
                        _copiedNodes.insert(std::pair<UiNodePtr, UiNodePtr>(_graphNodes[pos], nullptr));
                    }
                }
            }
            else if (ed::AcceptCut())
            {
                if (!readOnly())
                {
                    _copiedNodes.clear();
                    // same as copy but remove from graphNodes
                    for (ed::NodeId selected : selectedNodes)
                    {
                        int pos = findNode((int) selected.Get());
                        if (pos >= 0)
                        {
                            _copiedNodes.insert(std::pair<UiNodePtr, UiNodePtr>(_graphNodes[pos], nullptr));
                        }
                    }
                    _isCut = true;
                }
                else
                {
                    _popup = true;
                }
            }
            else if (ed::AcceptPaste())
            {
                if (!readOnly())
                {
                    for (std::map<UiNodePtr, UiNodePtr>::iterator iter = _copiedNodes.begin(); iter != _copiedNodes.end(); iter++)
                    {
                        copyUiNode(iter->first);
                    }
                    _addNewNode = true;
                }
                else
                {
                    _popup = true;
                }
            }
        }

        // set y position of first node
        std::vector<int> outputNum = createNodes(_isNodeGraph);

        // address copy information if applicable and relink graph if a new node has been added
        if (_addNewNode)
        {
            copyInputs();
            linkGraph();
            ImVec2 canvasPos = ed::ScreenToCanvas(mousePos);
            // place the copied nodes or the individual new nodes
            if ((int) _copiedNodes.size() > 0)
            {
                positionPasteBin(canvasPos);
            }
            else
            {
                ed::SetNodePosition(_graphNodes.back()->getId(), canvasPos);
            }
            _copiedNodes.clear();
            _addNewNode = false;
        }
        // layout and link graph during the initial call of drawGraph()
        if (_initial || _autoLayout)
        {
            _currLinks.clear();
            float y = 0.f;
            _levelMap = std::unordered_map<int, std::vector<UiNodePtr>>();
            // start layout with output or material nodes since layout algorithm works right to left
            for (int outN : outputNum)
            {
                layoutPosition(_graphNodes[outN], ImVec2(1200.f, y), true, 0);
                y += 350;
            }
            // if there are no output or material nodes but the nodes have position layout each individual node
            if (_graphNodes.size() > 0)
            {

                if (outputNum.size() == 0 && _graphNodes[0]->getMxElement())
                {
                    if (_graphNodes[0]->getMxElement()->hasAttribute("xpos"))
                    {
                        for (UiNodePtr node : _graphNodes)
                        {
                            layoutPosition(node, ImVec2(0, 0), true, 0);
                        }
                    }
                }
            }
            linkGraph();
            findYSpacing(0.f);
            layoutInputs();
            // automatically frame node graph upon loading
            ed::NavigateToContent();
        }
        if (_delete)
        {
            linkGraph();

            _delete = false;
        }
        connectLinks();
        // set to false after intial layout so that nodes can be moved
        _initial = false;
        _autoLayout = false;
        // delete selected nodes and their links if delete key is pressed or if the shortcut for cut is used
        if (ImGui::IsKeyReleased(GLFW_KEY_DELETE) || _isCut)
        {

            if (selectedNodes.size() > 0)
            {
                _frameCount = ImGui::GetFrameCount();
                _renderer->setMaterialCompilation(true);
                for (ed::NodeId id : selectedNodes)
                {

                    if (int(id.Get()) > 0)
                    {
                        int pos = findNode(int(id.Get()));
                        if (pos >= 0 && !readOnly())
                        {
                            deleteNode(_graphNodes[pos]);
                            _delete = true;
                            ed::DeselectNode(id);
                            ed::DeleteNode(id);
                            _currUiNode = nullptr;
                        }
                        else if (readOnly())
                        {
                            _popup = true;
                        }
                    }
                }
                linkGraph();
            }
            _isCut = false;
        }

        // start the session with content centered
        if (ImGui::GetFrameCount() == 2)
        {
            ed::NavigateToContent(0.0f);
        }

        // hotkey to frame selected node(s)
        if (ImGui::IsKeyReleased(GLFW_KEY_F) && !_fileDialogSave.IsOpened())
        {
            ed::NavigateToSelection();
        }

        // go back up from inside a subgraph
        if (ImGui::IsKeyReleased(GLFW_KEY_U) && (!ImGui::IsPopupOpen("add node")) && (!ImGui::IsPopupOpen("search")) && !_fileDialogSave.IsOpened())
        {
            upNodeGraph();
        }
        // adding new link
        if (ed::BeginCreate())
        {
            ed::PinId inputPinId, outputPinId, filterPinId;
            if (ed::QueryNewLink(&inputPinId, &outputPinId))
            {
                if (!readOnly())
                {

                    AddLink(inputPinId, outputPinId);
                }
                else
                {
                    _popup = true;
                }
            }
            if (ed::QueryNewNode(&filterPinId))
            {
                if (getPin(filterPinId)._type != "null")
                {
                    _pinFilterType = getPin(filterPinId)._type;
                }
            }
        }
        else
        {
            _pinFilterType = mx::EMPTY_STRING;
        }
        ed::EndCreate();
        // deleting link
        if (ed::BeginDelete())
        {
            ed::LinkId deletedLinkId;
            while (ed::QueryDeletedLink(&deletedLinkId))
            {
                if (!readOnly())
                {
                    deleteLink(deletedLinkId);
                }
                else
                {
                    _popup = true;
                }
            }
        }
        ed::EndDelete();
    }

    // diving into a node that has a subgraph
    ed::NodeId clickedNode = ed::GetDoubleClickedNode();
    if (clickedNode.Get() > 0)
    {
        if (_currUiNode != nullptr)
        {
            if (_currUiNode->getNode() != nullptr)
            {

                mx::InterfaceElementPtr impl = _currUiNode->getNode()->getImplementation();
                // only dive if current node is a node graph
                if (impl && impl->isA<mx::NodeGraph>())
                {
                    savePosition();
                    _graphStack.push(_graphNodes);
                    _pinStack.push(_currPins);
                    _sizeStack.push(_graphTotalSize);
                    mx::NodeGraphPtr implGraph = impl->asA<mx::NodeGraph>();
                    _initial = true;
                    _graphNodes.clear();
                    ed::DeselectNode(_currUiNode->getId());
                    _currUiNode = nullptr;
                    _currGraphElem = implGraph;
                    if (readOnly())
                    {
                        std::string graphName = implGraph->getName() + " (Read Only)";
                        _currGraphName.push_back(graphName);
                        _popup = true;
                    }
                    else
                    {

                        _currGraphName.push_back(implGraph->getName());
                    }
                    buildUiNodeGraph(implGraph);
                    ed::NavigateToContent();
                }
            }
            else if (_currUiNode->getNodeGraph() != nullptr)
            {
                savePosition();
                _graphStack.push(_graphNodes);
                _pinStack.push(_currPins);
                _sizeStack.push(_graphTotalSize);
                mx::NodeGraphPtr implGraph = _currUiNode->getNodeGraph();
                _initial = true;
                _graphNodes.clear();
                _isNodeGraph = true;
                setRenderMaterial(_currUiNode);
                ed::DeselectNode(_currUiNode->getId());
                _currUiNode = nullptr;
                _currGraphElem = implGraph;
                if (readOnly())
                {

                    std::string graphName = implGraph->getName() + " (Read Only)";
                    _currGraphName.push_back(graphName);
                    _popup = true;
                }
                else
                {
                    _currGraphName.push_back(implGraph->getName());
                }
                buildUiNodeGraph(implGraph);
                ed::NavigateToContent();
            }
        }
    }

    shaderPopup();
    if (ImGui::GetFrameCount() == (_frameCount + 2))
    {
        updateMaterials();
        _renderer->setMaterialCompilation(false);
    }

    ed::Suspend();
    _fileDialogSave.Display();
    // saving file
    if (_fileDialogSave.HasSelected())
    {

        std::string message;
        if (!_graphDoc->validate(&message))
        {
            std::cerr << "*** Validation warnings for " << _materialFilename.getBaseName() << " ***" << std::endl;
            std::cerr << message;
        }
        std::string fileName = _fileDialogSave.GetSelected().string();
        mx::FilePath name = _fileDialogSave.GetSelected().string();
        ed::Resume();
        savePosition();

        writeText(fileName, name);
        _fileDialogSave.ClearSelected();
    }
    else
    {
        ed::Resume();
    }

    ed::End();
    ImGui::End();
    _fileDialog.Display();
    // create and load document from selected file
    if (_fileDialog.HasSelected())
    {
        mx::FilePath fileName = mx::FilePath(_fileDialog.GetSelected().string());
        _currGraphName.clear();
        std::string graphName = fileName.getBaseName();
        _currGraphName.push_back(graphName.substr(0, graphName.length() - 5));
        _graphDoc = loadDocument(fileName);
        _graphDoc->importLibrary(_stdLib);
        
        _initial = true;
        buildUiBaseGraph(_graphDoc);
        _currGraphElem = _graphDoc;
        _prevUiNode = nullptr;
        _fileDialog.ClearSelected();

        _renderer->setDocument(_graphDoc);
        _renderer->updateMaterials(nullptr);
    }

    _fileDialogConstant.Display();
}

// return node location in graphNodes vector based off of node id
int Graph::findNode(int nodeId)
{
    int count = 0;
    for (size_t i = 0; i < _graphNodes.size(); i++)
    {
        if (_graphNodes[i]->getId() == nodeId)
        {
            return count;
        }
        count++;
    }
    return -1;
}

// find a link based on an attribute id
std::vector<int> Graph::findLinkId(int id)
{
    std::vector<int> ids;
    for (const Link& link : _currLinks)
    {
        if (link._startAttr == id || link._endAttr == id)
        {
            ids.push_back(link.id);
        }
    }
    return ids;
}
// check if current edge is already in edge vector
bool Graph::edgeExists(UiEdge newEdge)
{
    if (_currEdge.size() > 0)
    {
        for (UiEdge edge : _currEdge)
        {
            if (edge.getDown()->getId() == newEdge.getDown()->getId())
            {
                if (edge.getUp()->getId() == newEdge.getUp()->getId())
                {
                    if (edge.getInput() == newEdge.getInput())
                    {
                        return true;
                    }
                }
            }
            else if (edge.getUp()->getId() == newEdge.getDown()->getId())
            {
                if (edge.getDown()->getId() == newEdge.getUp()->getId())
                {
                    if (edge.getInput() == newEdge.getInput())
                    {
                        return true;
                    }
                }
            }
        }
    }
    else
    {
        return false;
    }
    return false;
}

// check if a link exists in currLink vector
bool Graph::linkExists(Link newLink)
{
    for (const auto& link : _currLinks)
    {
        if (link._startAttr == newLink._startAttr)
        {
            if (link._endAttr == newLink._endAttr)
            {
                // link exists
                return true;
            }
        }
        else if (link._startAttr == newLink._endAttr)
        {
            if (link._endAttr == newLink._startAttr)
            {
                // link exists
                return true;
            }
        }
    }
    return false;
}

// set materialX attribute positions for nodes which changed pos
void Graph::savePosition()
{
    for (UiNodePtr node : _graphNodes)
    {
        if (node->getMxElement() != nullptr)
        {
            ImVec2 pos = ed::GetNodePosition(node->getId());
            pos.x /= DEFAULT_NODE_SIZE.x;
            pos.y /= DEFAULT_NODE_SIZE.y;
            node->getMxElement()->setAttribute("xpos", std::to_string(pos.x));
            node->getMxElement()->setAttribute("ypos", std::to_string(pos.y));
            if (node->getMxElement()->hasAttribute("nodedef"))
            {
                node->getMxElement()->removeAttribute("nodedef");
            }
        }
    }
}
void Graph::writeText(std::string fileName, mx::FilePath filePath)
{
    if (filePath.getExtension() != mx::MTLX_EXTENSION)
    {
        filePath.addExtension(mx::MTLX_EXTENSION);
    }

    mx::XmlWriteOptions writeOptions;
    writeOptions.elementPredicate = getElementPredicate();
    mx::writeToXmlFile(_graphDoc, filePath, &writeOptions);
}