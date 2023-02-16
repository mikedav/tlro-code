#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Platform/Sdl2Application.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/ImageView.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/MatrixTransformation2D.h>
#include <Magnum/Primitives/Square.h>
#include <Magnum/ImGuiIntegration/Widgets.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Primitives/Square.h>
#include <fstream>
#include "nlohmann/json.hpp"
#include <gmpxx.h>
#include <string>
#include <iostream>
#include <fmt/format.h>
#include <fmt/color.h>
#include <SDL2/SDL.h>
#include <chrono>
#include <list>


using namespace Magnum;

using namespace Math::Literals;

using json = nlohmann::json;

using mpint = mpz_class;

using timepoint = std::chrono::time_point<std::chrono::system_clock>;



// Need this to draw bkg


typedef SceneGraph::Object<SceneGraph::MatrixTransformation2D> Object2D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation2D> Scene2D;

// Basically took it from my other project. It's fine...
class Drawable : SceneGraph::Drawable2D {
public:
    explicit Drawable(Object2D& object, SceneGraph::DrawableGroup2D& drawables, GL::Texture2D& texture, Shaders::FlatGL2D& shader): 
        SceneGraph::Drawable2D(object, &drawables), _texture(texture), _shader(&shader) {
            struct QuadVertex {
                Vector2 position;
                Vector2 textureCoordinates;
            };
            const QuadVertex vertices[]{
                {{ 1.f, -1.f}, {1.0f, 0.0f}}, /* Bottom right */
                {{ 1.f,  1.f}, {1.0f, 1.0f}}, /* Top right */
                {{-1.f, -1.f}, {0.0f, 0.0}}, /* Bottom left */
                {{-1.f,  1.f}, {0.0f, 1.0f}}  /* Top left */
            };
            _mesh.setPrimitive(GL::MeshPrimitive::TriangleStrip)
                .setCount(Containers::arraySize(vertices))
                .addVertexBuffer(GL::Buffer{vertices}, 0,
                    Magnum::Shaders::FlatGL2D::Position{},
                    Magnum::Shaders::FlatGL2D::TextureCoordinates{});
        };

    
private:
    void draw(const Matrix3& transformation, SceneGraph::Camera2D& camera) override {
        _shader->setTransformationProjectionMatrix(/*camera.projectionMatrix()*/ transformation).bindTexture(_texture).draw(_mesh);
    }

    Shaders::FlatGL2D* _shader;
    GL::Mesh _mesh;
    GL::Texture2D& _texture;
    
};



// Number formatting functions

std::string sci_format(mpint number) {
    std::string str_num = number.get_str();
    size_t num_digits = str_num.size();

    if (num_digits < 4)
        return str_num;

    return fmt::format("{:c}.{:c}{:c}e{:d}", str_num[0], str_num[1], str_num[2], num_digits - 1);
}

std::string engie_format(mpint number) {
    std::string str_num = number.get_str();
    size_t num_digits = str_num.size();

    if (num_digits < 4)
        return str_num;

    if (num_digits % 3 == 0)
        return fmt::format("{:c}{:c}{:c}e{:d}", str_num[0], str_num[1], str_num[2], num_digits - 3);

    if (num_digits % 3 == 1)
        return fmt::format("{:c}.{:c}{:c}e{:d}", str_num[0], str_num[1], str_num[2], num_digits - 1);

    return fmt::format("{:c}{:c}.{:c}e{:d}", str_num[0], str_num[1], str_num[2], num_digits - 2);
}


// Structs for game logic

struct Currency {
    std::string _name;
    mpint _count;
};

struct Generator : Currency {
    std::string _desc;
    mpint _output;
    Currency& _target;
    uint32_t _period_penalty;
    uint32_t _currentperiod = 0;

    Generator(std::string name, mpint count, const std::string& desc, mpint output, Currency& target, uint32_t period_penalty) : Currency{name, count},
        _desc(desc), _output(output), _target(target), _period_penalty(period_penalty) {  }
};

struct GeneratorGroup {
    std::string _name;
    std::list<Generator> _list;

    GeneratorGroup(std::string name) : _name(name), _list({}) { }
    GeneratorGroup(std::string name, std::list<Generator> list) : _name(name), _list(list) { }
};


// Placeholder names generator functions

// srand()? uniqueness guarantee?

std::string next_placeholder_gen_name() {
    return "Generator " + std::string{'A' + rand() % 26, '0' + rand() % 10, 'a' + rand() % 26};
}

std::string next_placeholder_gen_type_name() {
    return "Generator Type " + std::string{'A' + rand() % 26, '0' + rand() % 10, 'a' + rand() % 26};
}

// Price increase functions

mpint genByCount(mpint count) {

}


// App class

class Idle: public Platform::Application {
public:
    explicit Idle(const Arguments& arguments);

    void drawEvent() override;

    void viewportEvent(ViewportEvent& event) override;

    void keyPressEvent(KeyEvent& event) override;
    void keyReleaseEvent(KeyEvent& event) override;

    void mousePressEvent(MouseEvent& event) override;
    void mouseReleaseEvent(MouseEvent& event) override;
    void mouseMoveEvent(MouseMoveEvent& event) override;
    void mouseScrollEvent(MouseScrollEvent& event) override;
    void textInputEvent(TextInputEvent& event) override;
private:
    ImGuiIntegration::Context _imgui{NoCreate};

    // Game State

    Currency _cur1{"Currency 1", 10000};

    std::list<GeneratorGroup> _genGroups{{"Group A", {}}};

    std::list<std::string> _log;

    // Gui handles

    bool _cheatWindow = false;
    bool _menuWindow = false;

    int chAddValue = 0;
    int chSetValue = 0;

    bool _paused = false;

    // Json data and local settings copies for convinience
    json _consts;
    json _strings;
    json _config;
    json _layout;
    std::string _lang;

    uint32_t _w;
    uint32_t _h;

    // Game logic functions

    std::string locale(const std::string&);

    std::string num_format(mpint num) {
        return engie_format(num);
    }

    void timerInit();

    void update();

    void doMagic();

    void load() {
        std::cout << "loading not implemented\n";
    };

    void save() {
        std::cout << "saving not implemented\n";
    };

    bool purchase(mpint, Currency&, std::string);

    void earn(mpint, Currency&, std::string);

    // Time management vars

    uint32_t _extraUs = 0;
    timepoint _lastUpdate;

    // ImGui fonts

    ImFont* _font;
    ImFont* _smallFont;

    // Drawing stuff

    Shaders::FlatGL2D _shader{Shaders::FlatGL2D::Configuration{}
            .setFlags(Shaders::FlatGL2D::Flag::Textured)};
    GL::Texture2D _texture;
    Scene2D _scene;
    Object2D* _cameraObject;
    SceneGraph::Camera2D* _camera;
    SceneGraph::DrawableGroup2D _drawableGroup;
    Drawable* _bkgDrawable;

    // Delete later
    Containers::Optional<Trade::ImageData2D> _koreshImage;
    GL::Texture2D _koreshTexture;
};

// Game logic and logging functions 

std::string Idle::locale(const std::string& code) {
    return _strings.value(code, json::parse("{ \"en\" : \"MISSING\"}")).value(_lang, "NOTRANSLATION");
}

bool Idle::purchase(mpint price, Currency& cur, std::string reason) {
    if (cur._count < price) {
        _log.push_back(fmt::format("Purchase of {} failed: {} {}. missing {}", reason, price.get_str(), cur._name, (mpint(price - cur._count)).get_str()));
        return false;
    }

    cur._count -= price;
    _log.push_back(fmt::format("Purchase of {} successful: {} {}", reason, price.get_str(), cur._name));
    return true;
}

void Idle::earn(mpint amount, Currency& cur, std::string reason) {
    if (amount == 0)
        return;
    _log.push_back(fmt::format("Earned {} {} through {}", amount.get_str(), cur._name, reason));
    cur._count += amount;
}

void Idle::timerInit() {
    _lastUpdate = std::chrono::system_clock::now();
}

void Idle::doMagic() {
    if (_paused)
        return;


    for (auto& genGroup : _genGroups) {
        for (auto& generator : genGroup._list) {
            generator._currentperiod++;
            if (generator._currentperiod > (generator._period_penalty * static_cast<int>(_consts["global_period_penalty_factor"]))) {
                
                mpint total_output = generator._output * generator._count * mpint(static_cast<int>(_consts["global_output_factor"]));

                earn(total_output, 
                    generator._target, generator._name);

                generator._currentperiod = 0;
            }
        }
    }
}

void Idle::update() {
    uint32_t elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - _lastUpdate).count();
    _lastUpdate = std::chrono::system_clock::now();
    if ((_extraUs += elapsedUs) > 1000 * static_cast<uint32_t>(_consts["update_every_x_ms"])) {
        doMagic();
        _extraUs -= 1000 * static_cast<int>(_consts["update_every_x_ms"]);
    };

}

// App constructor

Idle::Idle(const Arguments& arguments): Platform::Application{arguments, Configuration{}}
{
    // read the json we need 
    std::ifstream constsStream("consts.json");
    std::ifstream stringsStream("strings.json");
    std::ifstream configStream("config.json");
    std::ifstream layoutStream("layout.json");
    _consts = json::parse(constsStream);
    _strings = json::parse(stringsStream);
    _config = json::parse(configStream);
    _layout = json::parse(layoutStream);

    // copy/cast values we need
    _lang = _config["lang"];
    _w = _config["w"];
    _h = _config["h"];

    // instantiate jpg loader
    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer =
        manager.loadAndInstantiate("JpegImporter");
    const Utility::Resource rs{"game-data"};

    // weird check but ok
    if(!importer || !importer->openData(rs.getRaw("bkg.jpg")))
        std::exit(1);

    // load background texture
    Containers::Optional<Trade::ImageData2D> bkgImage = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(bkgImage);

    _texture.setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::textureFormat(bkgImage->format()), bkgImage->size())
        .setSubImage(0, {}, *bkgImage);

    // load other textures
    if(!importer || !importer->openData(rs.getRaw("koresh.jpeg")))
        std::exit(1);

    _koreshImage = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(_koreshImage);
    _koreshTexture.setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::textureFormat(_koreshImage->format()), _koreshImage->size())
        .setSubImage(0, {}, *_koreshImage);

    // set up bkg drawable
    auto object = new Object2D(&_scene);

    _bkgDrawable = new Drawable(*object, _drawableGroup, _texture, _shader);


    // set up window according to config
    setWindowTitle(locale("app_name"));
    setWindowSize({_config["w"], _config["h"]});

    SDL_SetWindowFullscreen(window(), _config["fullscreen"] ? SDL_WINDOW_FULLSCREEN : 0);

    // set up camera and other drawing stuff,
    // ImGui context etc etc 
    _cameraObject = new Object2D{&_scene};
    _camera = new SceneGraph::Camera2D{*_cameraObject};
    _camera->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix3::projection({16.f/9.f, 1.f}))
        .setViewport(GL::defaultFramebuffer.viewport().size());
    _imgui = ImGuiIntegration::Context(Vector2{windowSize()} / dpiScaling(),
        windowSize(), framebufferSize());
    
    // Load and set font
    _font = ImGui::GetIO().Fonts->AddFontFromFileTTF("font.ttf", _config["font_size"]);
    _smallFont = ImGui::GetIO().Fonts->AddFontFromFileTTF("font.ttf", _config["small_font_size"]);

    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    timerInit();

}

// Main Loop

void Idle::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    
    // draw bkg
    _camera->draw(_drawableGroup);

    _imgui.newFrame();

    ImGui::PushFont(_font);
    
    // game logic
    update();
    // pass text input to Dear ImGui
    if(ImGui::GetIO().WantTextInput && !isTextInputActive())
        startTextInput();
    else if(!ImGui::GetIO().WantTextInput && isTextInputActive())
        stopTextInput();
    
    // show debug window
    if (_config["debug_window"]) {

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0/Double(ImGui::GetIO().Framerate), Double(ImGui::GetIO().Framerate));
    }

    // ImGui::ShowStackToolWindow();

    // Main area: generators etc
    {
        ImGuiWindowFlags window_flags = 0;

        window_flags |= ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoTitleBar;
        window_flags |= ImGuiWindowFlags_NoNav;
        window_flags |= ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({_w / 2, _h});

        ImGui::Begin(locale("main_window").c_str(), NULL, window_flags);

        // Menu window button
        if (ImGui::Button(locale("menu_window_button").c_str(), {static_cast<uint32_t>(_w / 2), 0}))
                _menuWindow ^= true;

        // _cur1 indicator
        ImGui::Text(num_format(_cur1._count).c_str()); ImGui::SameLine();

        // Manual increase button
        if (!_config["hide_manual"] && ImGui::Button(locale("manual_increase").c_str()))
            earn(static_cast<int>(_consts["manual_earnings"]), _cur1, "manual");

        // Loop through gen groups
        for (auto& genGroup : _genGroups) {
            
            // Label of gen group name
            ImGui::Text(genGroup._name.c_str());

            // std::string hashedName = fmt::format("genGroup##{}", genGroup._name).c_str();

            // Scroll buttons

            auto scroll_x_delta = static_cast<float>(_layout["scroll_x_delta"]);
            
            // Child window for each group
            ImGui::BeginChild(
                fmt::format("genGroup##{}", genGroup._name).c_str(), 
                {_w / 2 - 30, 200.f},
                true, 
                ImGuiWindowFlags_HorizontalScrollbar);

            ImGui::PopFont();
            ImGui::PushFont(_smallFont);
            
            // Loop through generators
            for (auto& generator : genGroup._list) {

                // Child window for each gen
                ImGui::BeginChild(
                    fmt::format("gen##{}{}", genGroup._name, generator._name).c_str(),
                    {170.f, 170.f}, 
                    true);
                
                // Label for number of generators 
                ImGui::Text(num_format(generator._count).c_str()); 


                // Buy new generator button
                if (ImGui::Button(generator._name.c_str(), {160, 0})) {
                    if (purchase(100, _cur1, "generator"))
                        generator._count++;
                }

                Magnum::ImGuiIntegration::image(_koreshTexture, {160, 100});
                
                ImGui::EndChild();
                ImGui::SameLine();
            }

            

            ImGui::PopFont();
            ImGui::PushFont(_font);

            // Price tag for next gen tier
            ImGui::Text("1000"); ImGui::SameLine();

            // Purchase button for next tier
            if (ImGui::Button(fmt::format("{}##{}", locale("purchase_next_gen_num_button"), genGroup._name).c_str())) {
                std::string next_gen_name = next_placeholder_gen_name();

                if (purchase(1000, _cur1, next_gen_name)) {
                    genGroup._list.emplace_back(
                        next_gen_name, 
                        1, 
                        "fff", 
                        1, 
                        genGroup._list.size() == 0 ? _cur1 : genGroup._list.back(), 
                        1);
                    
                };
            };

            ImGui::EndChild();

            // if (ImGui::SmallButton("<")){
            //     ImGui::BeginChild(hashedName);
            //     ImGui::SetScrollX(ImGui::GetScrollX() - scroll_x_delta);
            //     ImGui::EndChild();
            // };

            // if(ImGui::SmallButton(">")){
            //     ImGui::BeginChild(hashedName);
            //     ImGui::SetScrollX(ImGui::GetScrollX() + scroll_x_delta);
            //     ImGui::EndChild();
            // };
        }

        // Buy next gen type section

        if (_genGroups.front()._list.front()._count > 0) {    
            // Price tag for next generator type
            ImGui::Text("1000"); ImGui::SameLine();        

            // Button to nlock next generator type
            if (ImGui::Button(locale("purchase_next_gen_abc_button").c_str())) {
                std::string next_gen_type_name = next_placeholder_gen_type_name();
                if (purchase(1000, _cur1, next_gen_type_name)) {
                    Generator next_gen{next_placeholder_gen_name(), 1, "fff", 1, _cur1, 1};
                    _genGroups.emplace_back(next_gen_type_name, std::list<Generator>{next_gen});
                }
            }
        }

        ImGui::End();
    }

    // Menu
    if (_menuWindow) {
        ImGuiWindowFlags window_flags = 0;

        window_flags |= ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoNav;

        ImGui::SetNextWindowSize({_layout["menu_window_width"], _layout["menu_window_height"]});

        ImGui::Begin(locale("menu_window_title").c_str(), &_menuWindow, window_flags);

        // Pause button
        ImGui::SetCursorPosX((static_cast<uint32_t>(_layout["menu_window_width"]) - static_cast<uint32_t>(_layout["menu_button_width"])) / 2);

        if (ImGui::Button(
            _paused ? locale("unpause_button").c_str() : locale("pause_button").c_str(), 
            {static_cast<uint32_t>(_layout["menu_button_width"]), 0}))
            _paused ^= true;

        // Quit button
        ImGui::SetCursorPosX((static_cast<uint32_t>(_layout["menu_window_width"]) - static_cast<uint32_t>(_layout["menu_button_width"])) / 2);

        if (ImGui::Button(locale("exit").c_str(), {static_cast<uint32_t>(_layout["menu_button_width"]), 0}))
            exit();


        // Developer tool button
        if (_config["enable_cheating"]) {
            ImGui::SetCursorPosX((static_cast<uint32_t>(_layout["menu_window_width"]) - static_cast<uint32_t>(_layout["menu_button_width"])) / 2);
            if (ImGui::Button(locale("cheat_sheet").c_str(), {static_cast<uint32_t>(_layout["menu_button_width"]), 0}))
                _cheatWindow ^= true;
        }

        ImGui::End();
    }


    // Devtools window    
    if (_cheatWindow && _config["enable_cheating"]) {
        ImGuiWindowFlags window_flags = 0;

        window_flags |= ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoNav;
        window_flags |= ImGuiWindowFlags_NoCollapse;

        ImGui::SetNextWindowSize({_layout["cheat_sheet_width"], _layout["cheat_sheet_height"]});

        ImGui::Begin(locale("cheat_sheet").c_str(), &_cheatWindow, window_flags);

        // Add _cur1 
        ImGui::InputInt(locale("add_slider").c_str(), &chAddValue);
        ImGui::SameLine();
        if (ImGui::Button(locale("cheat_add").c_str()))
            earn(chAddValue, _cur1, "cheating");
        ImGui::InputInt(locale("set_slider").c_str(), &chSetValue);
        ImGui::SameLine();

        // Set _cur1
        if (ImGui::Button(locale("cheat_set").c_str())) {
            _cur1._count = chSetValue;
            _log.push_back(fmt::format("cur1 set to {} through cheating", chSetValue));
        }
        ImGui::End();
    }


    {
        ImGui::Begin(locale("log_window").c_str());
        for (auto& line : _log) {
            ImGui::Text(line.c_str());
        }
        ImGui::End();    
    }

    _imgui.updateApplicationCursor(*this);
    ImGui::PopFont();

    

    _imgui.drawFrame();
    
    

    

    swapBuffers();
    
    redraw();
}

// Transfer events to Dear ImGui. Basically final

void Idle::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    _imgui.relayout(Vector2{event.windowSize()}/event.dpiScaling(),
        event.windowSize(), event.framebufferSize());
}

void Idle::keyPressEvent(KeyEvent& event) {
    if(_imgui.handleKeyPressEvent(event)) return;
}

void Idle::keyReleaseEvent(KeyEvent& event) {
    if(_imgui.handleKeyReleaseEvent(event)) return;
}

void Idle::mousePressEvent(MouseEvent& event) {
    if(_imgui.handleMousePressEvent(event)) return;
}

void Idle::mouseReleaseEvent(MouseEvent& event) {
    if(_imgui.handleMouseReleaseEvent(event)) return;
}

void Idle::mouseMoveEvent(MouseMoveEvent& event) {
    if(_imgui.handleMouseMoveEvent(event)) return;
}

void Idle::mouseScrollEvent(MouseScrollEvent& event) {
    if(_imgui.handleMouseScrollEvent(event)) {
        event.setAccepted();
        return;
    }
}

void Idle::textInputEvent(TextInputEvent& event) {
    if(_imgui.handleTextInputEvent(event)) return;
}


MAGNUM_APPLICATION_MAIN(Idle)
