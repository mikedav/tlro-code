#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Platform/Sdl2Application.h>
#include <fstream>
#include "nlohmann/json.hpp"
#include <gmpxx.h>
#include <string>
#include <iostream>
#include <fmt/format.h>
#include <fmt/color.h>
#include <SDL2/SDL.h>
#include <chrono>
#include <vector>


using namespace Magnum;

using namespace Math::Literals;

using json = nlohmann::json;

using mpint = mpz_class;

using timepoint = std::chrono::time_point<std::chrono::system_clock>;

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
        _desc(desc), _output(output), _target(target), _period_penalty(period_penalty) {}
};



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

    Currency _cur1{"Currency 1", 0};

    Generator _a1gen{"A1", 0, "fff", 1, _cur1, 1};

    std::vector<std::vector<Generator>> _genGroups{{_a1gen}};

    std::vector<std::string> _log;

    bool _cheatWindow = false;

    int chAddValue = 0;
    int chSetValue = 0;

    json _consts;
    json _strings;
    json _config;
    json _layout;
    std::string _lang;

    uint32_t _w;
    uint32_t _h;

    std::string locale(const std::string&);

    void timerInit();

    void update();

    void doMagic();

    void load() {
        std::cout << "loading not implemented\n";
    };

    void save() {
        std::cout << "saving not implemented\n";
    };

    bool purchase(mpint, Currency&, const std::string&);

    void earn(mpint, Currency&, const std::string&);

    uint32_t extraUs = 0;
    timepoint lastUpdate;

    ImGuiWindowFlags imwf_static;
};

bool Idle::purchase(mpint price, Currency& cur, const std::string& reason) {
    if (cur._count < price) {
        _log.push_back(fmt::format("Purchase of {} failed: {} {}. missing {}", reason, price.get_str(), cur._name, (mpint(price - cur._count)).get_str()));
        return false;
    }

    cur._count -= price;
    _log.push_back(fmt::format("Purchase of {} successful: {} {}", reason, price.get_str(), cur._name));
    return true;
}

void Idle::earn(mpint amount, Currency& cur, const std::string& reason) {
    _log.push_back(fmt::format("Earned {} {} through {}", amount.get_str(), cur._name, reason));
    cur._count += amount;
}

void Idle::timerInit() {
    lastUpdate = std::chrono::system_clock::now();
}

void Idle::doMagic() {
    for (auto& genGroup : _genGroups) {
        for (auto& generator : genGroup) {
            generator._currentperiod++;
            if (generator._currentperiod > generator._period_penalty * static_cast<int>(_consts["global_period_penalty_factor"])) {
                earn(generator._output * generator._count * mpint(static_cast<int>(_consts["global_output_factor"])), generator._target, generator._name);
                generator._currentperiod = 0;
            }
        }
    }
}

void Idle::update() {
    uint32_t elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - lastUpdate).count();
    lastUpdate = std::chrono::system_clock::now();
    if ((extraUs += elapsedUs) > 1000 * static_cast<uint32_t>(_consts["update_every_x_ms"])) {
        fmt::print(fg(fmt::color::crimson), "doing magic\n");
        
        doMagic();

        extraUs -= 1000 * static_cast<int>(_consts["update_every_x_ms"]);
        std::cout << "extraUs" << extraUs << "\n";
    };

}

Idle::Idle(const Arguments& arguments): Platform::Application{arguments, Configuration{}}
{
    std::ifstream constsStream("consts.json");
    std::ifstream stringsStream("strings.json");
    std::ifstream configStream("config.json");
    std::ifstream layoutStream("layout.json");
    _consts = json::parse(constsStream);
    _strings = json::parse(stringsStream);
    _config = json::parse(configStream);
    _layout = json::parse(layoutStream);
    _lang = _config["lang"];
    _w = _config["w"];
    _h = _config["h"];

    setWindowTitle(locale("app_name"));
    setWindowSize({_config["w"], _config["h"]});

    SDL_SetWindowFullscreen(window(), _config["fullscreen"] ? SDL_WINDOW_FULLSCREEN : 0);

    imwf_static = 0;
    imwf_static |= ImGuiWindowFlags_NoMove;
    imwf_static |= ImGuiWindowFlags_NoResize;

    _imgui = ImGuiIntegration::Context(Vector2{windowSize()} / dpiScaling(),
        windowSize(), framebufferSize());
    ImGui::GetIO().FontGlobalScale = _config["font_scale"];
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

void Idle::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    _imgui.newFrame();

    update();

    if(ImGui::GetIO().WantTextInput && !isTextInputActive())
        startTextInput();
    else if(!ImGui::GetIO().WantTextInput && isTextInputActive())
        stopTextInput();

    if (_config["debug_window"]) {

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0/Double(ImGui::GetIO().Framerate), Double(ImGui::GetIO().Framerate));
    }

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
        ImGui::Text(engie_format(_cur1._count).c_str());
        if (ImGui::Button(locale("manual_increase").c_str()))
            earn(static_cast<int>(_consts["manual_earnings"]), _cur1, "manual");

        if (_config["enable_cheating"] && ImGui::Button(locale("cheat_sheet").c_str()))
            _cheatWindow ^= true;

        for (auto& genGroup : _genGroups) {
            ImGui::BeginChild("genGroup", {200.f, 200.f}, NULL, imwf_static);
            for (auto& generator : genGroup) {
                ImGui::Text(generator._name.c_str()); ImGui::SameLine(); 
                if (ImGui::Button(generator._count.get_str().c_str())) {
                    if (purchase(100, _cur1, "generator"))
                        generator._count++;
                }
            }
            ImGui::EndChild();
        }
        
        ImGui::SetCursorPos({(_w / 2) - 60, _h - 35});
        if (ImGui::Button(locale("exit").c_str()))
            exit();

        ImGui::End();
    }

    if (_cheatWindow && _config["enable_cheating"]) {
        ImGuiWindowFlags window_flags = 0;

        window_flags |= ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoNav;
        window_flags |= ImGuiWindowFlags_NoCollapse;

        ImGui::SetNextWindowSize({_layout["cheat_sheet_width"], _layout["cheat_sheet_height"]}, ImGuiCond_FirstUseEver);

        ImGui::Begin(locale("cheat_sheet").c_str(), &_cheatWindow, window_flags);
        ImGui::InputInt(locale("add_slider").c_str(), &chAddValue);
        ImGui::SameLine();
        if (ImGui::Button(locale("cheat_add").c_str()))
            earn(chAddValue, _cur1, "cheating");
        ImGui::InputInt(locale("set_slider").c_str(), &chSetValue);
        ImGui::SameLine();
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
    _imgui.drawFrame();
    swapBuffers();
    redraw();
}

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

std::string Idle::locale(const std::string& code) {
    return _strings.value(code, json::parse("{ \"en\" : \"MISSING\"}")).value(_lang, "NOTRANSLATION");
}

MAGNUM_APPLICATION_MAIN(Idle)
