#include "editor.h"

// #include "vxng/renderer.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_video.h>
#include <sdl3webgpu.h>
#include <webgpu/webgpu.hpp>

#include <cstdlib>

Editor::Editor()
    // : renderer(), viewport_camera()
    = default;

auto Editor::init() -> int {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Initialization failed: %s", SDL_GetError());
        return EXIT_FAILURE;
    };

    // create window
    this->sdl_window = SDL_CreateWindow("My Funny Window", 800, 600, 0
                                        // SDL_WINDOW_RESIZABLE
    );
    if (!sdl_window) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM,
                     "Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // setup webgpu instance
    wgpu::InstanceDescriptor instance_desc;
    instance_desc.nextInChain = nullptr;
    wgpu::Instance wgpu_instance = wgpu::createInstance(instance_desc);

    // surface should be fully configured for us
    this->wgpu.surface = SDL_GetWGPUSurface(wgpu_instance, this->sdl_window);

    // request adapter
    wgpu::RequestAdapterOptions adapter_options;
    adapter_options.nextInChain = nullptr;
    adapter_options.compatibleSurface = this->wgpu.surface;
    auto adapter = wgpu_instance.requestAdapter(adapter_options);

    wgpu_instance.release();

    // request device from adapter
    wgpu::DeviceDescriptor device_desc;
    device_desc.nextInChain = nullptr;
    device_desc.label = "My Device";
    device_desc.requiredFeatureCount = 0;
    device_desc.requiredLimits = nullptr;
    device_desc.defaultQueue.nextInChain = nullptr;
    device_desc.defaultQueue.label = "The default queue";
    device_desc.deviceLostCallback = [](WGPUDeviceLostReason reason,
                                        char const *message,
                                        void * /* pUserData */) {
        std::cout << "Device lost: reason " << reason;
        if (message)
            std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    this->wgpu.device = adapter.requestDevice(device_desc);

    auto onDeviceError = [](wgpu::ErrorType type, char const *message) {
        std::cout << "Uncaptured device error: type " << type;
        if (message)
            std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    this->wgpu.device.setUncapturedErrorCallback(onDeviceError);

    // get queue from device
    this->wgpu.queue = this->wgpu.device.getQueue();

    /*
        // TODO: webgpuize

        // setup our renderer
        if (!this->renderer.init_gl()) {
            return EXIT_FAILURE;
        }
        this->viewport_camera.init_gl();

        // set initial renderer size
        int width, height;
        SDL_GetWindowSize(sdl_window, &width, &height);
        this->renderer.resize(width, height);

        this->renderer.set_active_camera(&this->viewport_camera);
    */

    adapter.release();

    return 0;
}

Editor::~Editor() {
    if (this->wgpu.surface)
        this->wgpu.surface.unconfigure();

    SDL_Quit();
}

auto Editor::loop() -> void {
    bool quit = false;
    SDL_Event evt;
    while (!quit) {
        // catch sdl events
        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {

            case SDL_EVENT_QUIT:
                quit = true;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                handle_resize(evt.window.data1, evt.window.data2);
                break;

            case SDL_EVENT_KEY_DOWN:
                if (evt.key.key == SDLK_ESCAPE) {
                    quit = true;
                    break;
                }

            case SDL_EVENT_MOUSE_MOTION:
                handle_mouse_motion(evt.motion);
                break;
            }
        }

        // render all my things
        // TODO: webgpuize
        // glClearColor(0.f, 0.f, 0.f, 1.f);
        // glClear(GL_COLOR_BUFFER_BIT);
        // this->renderer.render();
    }
}

auto Editor::handle_resize(int width, int height) -> void {
    // TODO: webgpuize
    // glViewport(0, 0, width, height);

    // update info for shaders etc
    this->renderer.resize(width, height);
}

auto Editor::handle_mouse_motion(SDL_MouseMotionEvent event) -> void {
    // only control camera if alt is held
    SDL_Keymod mods = SDL_GetModState();
    if (!(mods & SDL_KMOD_ALT)) {
        return;
    }

    if (event.state & SDL_BUTTON_LMASK) {
        this->viewport_camera.handle_rotation(event.xrel, event.yrel);
    } else if (event.state & SDL_BUTTON_RMASK) {
        this->viewport_camera.handle_zoom(event.yrel);
    } else if (event.state & SDL_BUTTON_MMASK) {
        this->viewport_camera.handle_pan(event.xrel, event.yrel);
    }
}
