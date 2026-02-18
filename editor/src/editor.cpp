#include "editor.h"
#include "SDL3/SDL_video.h"

#include <sdl3webgpu.h>

#include <cstdlib>
#include <iostream>

Editor::Editor()
    : renderer() //, viewport_camera()
{};

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
    wgpu::InstanceDescriptor instance_desc = {};
    instance_desc.nextInChain = nullptr;
    wgpu::InstanceFeatureName instance_features[] = {
        wgpu::InstanceFeatureName::TimedWaitAny};
    instance_desc.requiredFeatures = instance_features;
    instance_desc.requiredFeatureCount = 1;
    wgpu::Instance wgpu_instance = wgpu::CreateInstance(&instance_desc);

    // surface should be fully configured for us
    auto surface = SDL_GetWGPUSurface(wgpu_instance.Get(), this->sdl_window);
    this->wgpu.surface = wgpu::Surface::Acquire(surface);

    // request adapter
    wgpu::RequestAdapterOptions adapter_options = {};
    adapter_options.nextInChain = nullptr;
    adapter_options.compatibleSurface = this->wgpu.surface;
    wgpu::Adapter adapter = nullptr;
    wgpu::Future future = wgpu_instance.RequestAdapter(
        &adapter_options, wgpu::CallbackMode::WaitAnyOnly,
        [&adapter](wgpu::RequestAdapterStatus status, wgpu::Adapter a,
                   wgpu::StringView message) {
            if (status == wgpu::RequestAdapterStatus::Success) {
                adapter = std::move(a);
            } else {
                std::cerr << "Failed to get adapter: "
                          << std::string(message.data, message.length)
                          << std::endl;
            }
        });

    // block until the adapter request completes
    wgpu::WaitStatus waitStatus = wgpu_instance.WaitAny(future, UINT64_MAX);
    if (waitStatus != wgpu::WaitStatus::Success || !adapter) {
        std::cerr << "Failed to wait for adapter" << std::endl;
        return EXIT_FAILURE;
    }

    // request device from adapter
    wgpu::DeviceDescriptor device_desc = {};
    device_desc.nextInChain = nullptr;
    device_desc.label = "My Device";
    device_desc.requiredFeatureCount = 0;
    device_desc.requiredLimits = nullptr;
    device_desc.defaultQueue.nextInChain = nullptr;
    device_desc.defaultQueue.label = "The default queue";
    device_desc.SetDeviceLostCallback(
        wgpu::CallbackMode::AllowSpontaneous,
        [](const wgpu::Device &, wgpu::DeviceLostReason reason,
           wgpu::StringView message) {
            std::cerr << "Device lost: reason "
                      << static_cast<uint32_t>(reason);
            if (message.length > 0)
                std::cerr << " (" << std::string(message.data, message.length)
                          << ")";
            std::cerr << std::endl;
        });
    device_desc.SetUncapturedErrorCallback([](const wgpu::Device &,
                                              wgpu::ErrorType type,
                                              wgpu::StringView message) {
        std::cerr << "Uncaptured device error: type "
                  << static_cast<uint32_t>(type);
        if (message.length > 0)
            std::cerr << " (" << std::string(message.data, message.length)
                      << ")";
        std::cerr << std::endl;
    });
    future = adapter.RequestDevice(
        &device_desc, wgpu::CallbackMode::WaitAnyOnly,
        [this](wgpu::RequestDeviceStatus status, wgpu::Device d,
               wgpu::StringView message) {
            if (status == wgpu::RequestDeviceStatus::Success) {
                this->wgpu.device = std::move(d);
            } else {
                std::cerr << "Failed to get device: "
                          << std::string(message.data, message.length)
                          << std::endl;
            }
        });

    // block until the device request completes
    waitStatus = wgpu_instance.WaitAny(future, UINT64_MAX);
    if (waitStatus != wgpu::WaitStatus::Success || !adapter) {
        std::cerr << "Failed to wait for adapter" << std::endl;
        return EXIT_FAILURE;
    }

    // get queue from device
    this->wgpu.queue = this->wgpu.device.GetQueue();

    // configure the surface
    wgpu::SurfaceConfiguration config = {};

    // configuration of the textures created for the underlying swap chain
    config.width = 640;
    config.height = 480;
    config.usage = wgpu::TextureUsage::RenderAttachment;
    wgpu::SurfaceCapabilities capabilities;
    this->wgpu.surface.GetCapabilities(adapter, &capabilities);
    config.format = capabilities.formats[0]; // supposedly this is preferred

    // and we do not need any particular view format:
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;
    config.device = this->wgpu.device;
    config.presentMode = wgpu::PresentMode::Fifo;
    config.alphaMode = wgpu::CompositeAlphaMode::Auto;

    this->wgpu.surface.Configure(&config);

    if (!this->renderer.init_webgpu(&this->wgpu.device)) {
        return EXIT_FAILURE;
    }

    // set initial renderer size
    int width, height;
    SDL_GetWindowSize(sdl_window, &width, &height);
    this->renderer.resize(width, height);

    /*
        // TODO: webgpuize
        this->viewport_camera.init_gl();
        this->renderer.set_active_camera(&this->viewport_camera);
    */

    return 0;
}

Editor::~Editor() {
    this->wgpu.surface.Unconfigure();

    SDL_Quit();
}

auto Editor::run() -> void {
    bool quit = false;
    while (!quit) {
        poll_events(quit);
        draw_to_surface();
    }
}

auto Editor::draw_to_surface() -> void {
    // Get the next target texture view
    wgpu::TextureView targetView = get_next_surface_texture_view();
    if (!targetView)
        return;

    // Create a command encoder for the draw call
    wgpu::CommandEncoderDescriptor encoderDesc = {};
    encoderDesc.label = "My command encoder";
    wgpu::CommandEncoder encoder =
        this->wgpu.device.CreateCommandEncoder(&encoderDesc);

    // Create the render pass that clears the screen with our color
    wgpu::RenderPassDescriptor renderPassDesc = {};

    // The attachment part of the render pass descriptor describes the target
    // texture of the pass
    wgpu::RenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view = targetView;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = wgpu::LoadOp::Clear;
    renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
    renderPassColorAttachment.clearValue = wgpu::Color{0.0, 0.0, 0.0, 1.0};

    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites = nullptr;

    // render pass!
    wgpu::RenderPassEncoder renderPass =
        encoder.BeginRenderPass(&renderPassDesc);

    // delegate this to our vxng::Renderer
    renderer.render(renderPass);

    renderPass.End();

    // Finally encode and submit the render pass
    wgpu::CommandBufferDescriptor cmdBufferDescriptor = {};
    cmdBufferDescriptor.label = "Command buffer";
    wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);

    this->wgpu.queue.Submit(1, &command);

#if defined(WEBGPU_BACKEND_DAWN)
    this->wgpu.device.Tick();
#elif defined(WEBGPU_BACKEND_WGPU)
    this->wgpu.device.Poll(false);
#endif

    // Present the surface texture to display it on the window
    this->wgpu.surface.Present();
}

auto Editor::get_next_surface_texture_view() -> wgpu::TextureView {
    // get the surface texture
    wgpu::SurfaceTexture surface_texture;
    this->wgpu.surface.GetCurrentTexture(&surface_texture);
    if (!(surface_texture.status ==
              wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal ||
          surface_texture.status ==
              wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal)) {
        return nullptr;
    }
    wgpu::Texture texture = surface_texture.texture;

    // Create a view for this surface texture
    wgpu::TextureViewDescriptor texview_desc;
    texview_desc.nextInChain = nullptr;
    texview_desc.label = "Surface texture view";
    texview_desc.format = texture.GetFormat();
    texview_desc.dimension = wgpu::TextureViewDimension::e2D;
    texview_desc.baseMipLevel = 0;
    texview_desc.mipLevelCount = 1;
    texview_desc.baseArrayLayer = 0;
    texview_desc.arrayLayerCount = 1;
    texview_desc.aspect = wgpu::TextureAspect::All;

    wgpu::TextureView target_view = texture.CreateView(&texview_desc);

    return target_view;
}

auto Editor::poll_events(bool &quit) -> void {
    SDL_Event evt;
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
}

auto Editor::handle_resize(int width, int height) -> void {
    /*
        // TODO: webgpuize
        glViewport(0, 0, width, height);

        // update info for shaders etc
        this->renderer.resize(width, height);
    */
}

auto Editor::handle_mouse_motion(SDL_MouseMotionEvent event) -> void {
    // only control camera if alt is held
    SDL_Keymod mods = SDL_GetModState();
    if (!(mods & SDL_KMOD_ALT)) {
        return;
    }

    /*
        // TODO: bring back
        if (event.state & SDL_BUTTON_LMASK) {
            this->viewport_camera.handle_rotation(event.xrel, event.yrel);
        } else if (event.state & SDL_BUTTON_RMASK) {
            this->viewport_camera.handle_zoom(event.yrel);
        } else if (event.state & SDL_BUTTON_MMASK) {
            this->viewport_camera.handle_pan(event.xrel, event.yrel);
        }
    */
}
