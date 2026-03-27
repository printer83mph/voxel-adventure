#include "editor.h"

#include "cursors.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_wgpu.h>
#include <sdl3webgpu.h>
#include <vxng/vxng.h>
#include <webgpu/webgpu_cpp.h>

#include <cstdlib>
#include <iostream>

#define SCENE_RESOLUTION 512
#define DEFAULT_SCENE_SCALE 32.f

Editor::Editor()
    : renderer(), viewport_camera(),
      scene(std::make_unique<vxng::scene::Scene>(SCENE_RESOLUTION,
                                                 DEFAULT_SCENE_SCALE)),
      cursors(), tools(), current_tool(&tools.voxel_brush) {};

auto Editor::init() -> int {

    // init imgui context
    IMGUI_CHECKVERSION();
    this->imgui_context = ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Initialization failed: %s", SDL_GetError());
        return EXIT_FAILURE;
    };

    // create window
    this->sdl_window =
        SDL_CreateWindow("Voxel Editor", 800, 600, SDL_WINDOW_RESIZABLE);
    if (!sdl_window) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM,
                     "Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // setup sdl cursors
    this->cursors.setup_sdl_cursors();

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
    if (waitStatus != wgpu::WaitStatus::Success || !this->wgpu.device) {
        std::cerr << "Failed to wait for device" << std::endl;
        return EXIT_FAILURE;
    }

    // get queue from device
    this->wgpu.queue = this->wgpu.device.GetQueue();

    wgpu::SurfaceCapabilities capabilities;
    this->wgpu.surface.GetCapabilities(adapter, &capabilities);
    this->wgpu.preferred_format =
        capabilities.formats[0]; // supposedly this is preferred

    int width, height;
    SDL_GetWindowSize(sdl_window, &width, &height);

    // configure the surface for the underlying swap chain
    wgpu::SurfaceConfiguration config =
        get_surface_configuration(width, height);
    this->wgpu.surface.Configure(&config);

    // ok now we init members!

    if (!this->renderer.init_webgpu(this->wgpu.device)) {
        return EXIT_FAILURE;
    }

    // set initial renderer size (shader globals)
    this->renderer.resize(width, height);

    this->viewport_camera.init_webgpu(this->wgpu.device);
    this->viewport_camera.set_aspect_ratio(static_cast<float>(width) / height);
    this->renderer.set_active_camera(&this->viewport_camera);

    this->scene->init_webgpu(this->wgpu.device);
    this->scene->fill_basic_plane({255, 255, 255, 255});
    this->renderer.set_scene(this->scene.get());

    // initialize imgui
    ImGui_ImplSDL3_InitForOther(this->sdl_window);

    ImGui_ImplWGPU_InitInfo init_info;
    init_info.Device = this->wgpu.device.Get();
    init_info.RenderTargetFormat =
        (WGPUTextureFormat)this->wgpu.preferred_format;
    init_info.DepthStencilFormat =
        (WGPUTextureFormat)wgpu::TextureFormat::Depth32Float;
    ImGui_ImplWGPU_Init(&init_info);

    return 0;
}

Editor::~Editor() {
    if (this->imgui_context != nullptr) {
        ImGui::SetCurrentContext(this->imgui_context);
        ImGui_ImplWGPU_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext(this->imgui_context);
        this->imgui_context = nullptr;
    }

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

    // render UI
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    this->run_gui();
    ImGui::Render();

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

    // depth attachment for multi-chunk rendering
    wgpu::RenderPassDepthStencilAttachment depthAttachment;
    depthAttachment.view = renderer.get_depth_texture_view();
    depthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
    depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
    depthAttachment.depthClearValue = 1.0f; // far plane
    depthAttachment.stencilLoadOp = wgpu::LoadOp::Undefined;
    depthAttachment.stencilStoreOp = wgpu::StoreOp::Undefined;

    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;
    renderPassDesc.depthStencilAttachment = &depthAttachment;
    renderPassDesc.timestampWrites = nullptr;

    // render pass!
    wgpu::RenderPassEncoder renderPass =
        encoder.BeginRenderPass(&renderPassDesc);

    // delegate this to our vxng::Renderer
    renderer.render(renderPass);

    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass.Get());

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

auto Editor::run_gui() -> void {

    // --------- Title/File menu ---------

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New (Empty)")) {
                this->new_empty_scene();
            }
            if (ImGui::BeginMenu("New (Templates)")) {
                if (ImGui::MenuItem("Flat Plane")) {
                    this->new_empty_scene();
                    this->scene->fill_basic_plane({255, 255, 255, 255});
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Open")) {
                this->handle_open_vox_file();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Panels")) {
            if (ImGui::MenuItem("Tools", NULL, this->panels.show_tools))
                this->panels.show_tools = !this->panels.show_tools;

            if (ImGui::MenuItem("Options", NULL, this->panels.show_options))
                this->panels.show_options = !this->panels.show_options;

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // --------- Options menu ---------

    if (this->panels.show_options) {
        ImGui::Begin("Options");
        if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {

            // Scene Resolution
            float scale = this->scene->get_chunk_scale();
            int unit_voxel_depth = glm::log2(512.f / scale);
            int original_uvd = unit_voxel_depth;
            ImGui::SliderInt("Resolution", &unit_voxel_depth, 1, 5);
            ImGui::TextWrapped(
                "This is measured in \"unit voxel depth\", i.e. how "
                "many depths of octrees inhabit one unit of space.");
            if (unit_voxel_depth != original_uvd) {
                // snap scale to powers of 2
                float new_scale = 512.f / glm::pow(2.f, unit_voxel_depth);
                this->scene->set_chunk_scale(new_scale);
            }
        }
        ImGui::End();
    }

    // --------- Tool menu ---------

    if (this->panels.show_tools) {
        ImGui::Begin("Tools");

        bool is_voxel_brush_selected =
            this->current_tool == &this->tools.voxel_brush;
        if (ImGui::RadioButton("Voxel Brush", is_voxel_brush_selected))
            this->current_tool = &this->tools.voxel_brush;
        ImGui::TextWrapped("More tools on the way!");

        // spacer
        ImGui::Dummy(ImVec2(0.0f, 24.0f));

        // tool options section
        auto tool_name = std::string(this->current_tool->get_tool_name());
        if (ImGui::CollapsingHeader((tool_name + "###ToolMenu").c_str(),
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            this->current_tool->render_ui();
        }
        ImGui::End();
    }
}

const SDL_DialogFileFilter Editor::vox_filters[] = {
    {.name = "MagicaVoxel files", .pattern = "vox"},
};

auto Editor::handle_open_vox_file() -> void {
    SDL_ShowOpenFileDialog(&open_vox_file, this, this->sdl_window, vox_filters,
                           1, NULL, false);
}

auto Editor::open_vox_file(void *user_data, const char *const *file_list,
                           int filter) -> void {
    if (!file_list) {
        SDL_Log("An error occured: %s", SDL_GetError());
        return;
    } else if (!*file_list) {
        SDL_Log("The user did not select any file.");
        SDL_Log("Most likely, the dialog was canceled.");
        return;
    }

    while (*file_list) {
        SDL_Log("Full path to selected file: '%s'", *file_list);
        file_list++;
    }
}

auto Editor::get_surface_configuration(int width, int height)
    -> wgpu::SurfaceConfiguration {
    wgpu::SurfaceConfiguration config = {};
    config.width = width;
    config.height = height;
    config.usage = wgpu::TextureUsage::RenderAttachment;
    config.format = this->wgpu.preferred_format;
    config.viewFormatCount = 0; // and we do not need any particular view format
    config.viewFormats = nullptr;
    config.device = this->wgpu.device;
    config.presentMode = wgpu::PresentMode::Fifo;
    config.alphaMode = wgpu::CompositeAlphaMode::Auto;
    return config;
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

        // do imgui events first
        ImGui_ImplSDL3_ProcessEvent(&evt);
        ImGuiIO &io = ImGui::GetIO();

        switch (evt.type) {

        case SDL_EVENT_QUIT:
            quit = true;
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            handle_resize(evt.window.data1, evt.window.data2);
            break;

        case SDL_EVENT_KEY_DOWN:
            if (io.WantCaptureKeyboard) // imgui takes precedence
                break;
            handle_key_down(evt.key, &quit);
            break;

        case SDL_EVENT_KEY_UP:
            if (io.WantCaptureKeyboard) // imgui takes precedence
                break;
            handle_key_up(evt.key);
            break;

        case SDL_EVENT_MOUSE_MOTION:
            if (io.WantCaptureMouse) // imgui takes precedence
                break;
            handle_mouse_motion(evt.motion);
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (io.WantCaptureMouse) // imgui takes precedence
                break;
            handle_mouse_down(evt.button);
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (io.WantCaptureMouse) // imgui takes precedence
                break;
            handle_mouse_up(evt.button);
            break;
        }
    }
}

auto Editor::handle_resize(int width, int height) -> void {
    // reconfigure the surface with new dimensions
    wgpu::SurfaceConfiguration config =
        get_surface_configuration(width, height);
    this->wgpu.surface.Configure(&config);

    // update info for shaders etc
    this->renderer.resize(width, height);
    this->viewport_camera.set_aspect_ratio(static_cast<float>(width) / height);
}

auto random_float() -> float {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

auto Editor::handle_key_down(const SDL_KeyboardEvent &event, bool *quit)
    -> void {
    switch (event.key) {
    case SDLK_ESCAPE:
        *quit = true;
        break;

    case SDLK_A: {
        // add random white block
        auto rand_pos =
            (glm::vec3(random_float(), random_float(), random_float()) -
             glm::vec3(0.5f)) *
            0.99f * this->scene->get_chunk_scale();
        auto rand_depth = rand() % 3 + 1;
        auto color = glm::u8vec4(255, 255, 255, 255);
        this->scene->set_voxel_filled(rand_depth, rand_pos, color);
        break;
    }

    case SDLK_D: {
        // delete random block
        auto rand_pos =
            (glm::vec3(random_float(), random_float(), random_float()) -
             glm::vec3(0.5f)) *
            0.99f * this->scene->get_chunk_scale();
        auto rand_depth = rand() % 3 + 1;
        this->scene->set_voxel_empty(rand_depth, rand_pos);
        break;
    }

    default: {
        // pass off to current tool
        glm::vec2 mouse_ndc_pos = get_mouse_ndc_coords();
        this->current_tool->handle_keyboard_event(
            EditorTool::KeyboardEventBundle{
                .event = &event,
                .mouse_ndc_coords = mouse_ndc_pos,
                .scene = this->scene.get(),
                .camera = &this->viewport_camera,
                .cursors = &this->cursors,
            });
    }
    }
}

auto Editor::handle_key_up(const SDL_KeyboardEvent &event) -> void {
    // pass off to current tool
    glm::vec2 mouse_ndc_pos = get_mouse_ndc_coords();
    this->current_tool->handle_keyboard_event(EditorTool::KeyboardEventBundle{
        .event = &event,
        .mouse_ndc_coords = mouse_ndc_pos,
        .scene = this->scene.get(),
        .camera = &this->viewport_camera,
        .cursors = &this->cursors,
    });
}

auto Editor::handle_mouse_motion(const SDL_MouseMotionEvent &event) -> void {
    SDL_Keymod mods = SDL_GetModState();

    // camera movement (only if alt is held)
    if (mods & SDL_KMOD_ALT) {
        if (event.state & SDL_BUTTON_LMASK) {
            this->viewport_camera.handle_rotation(event.xrel, event.yrel);
            this->cursors.set_cursor(Cursors::Variant::MOVE);
        } else if (event.state & SDL_BUTTON_RMASK) {
            this->viewport_camera.handle_zoom(event.yrel);
            this->cursors.set_cursor(Cursors::Variant::NS_RESIZE);
        } else if (event.state & SDL_BUTTON_MMASK) {
            this->viewport_camera.handle_pan(event.xrel, event.yrel);
            this->cursors.set_cursor(Cursors::Variant::MOVE);
        }
    } else {
        // no mod held, pass off to current tool
        glm::vec2 mouse_ndc_pos = get_mouse_ndc_coords();
        this->current_tool->handle_mouse_motion_event(
            EditorTool::MouseMotionEventBundle{
                .event = &event,
                .mouse_ndc_coords = mouse_ndc_pos,
                .scene = this->scene.get(),
                .camera = &this->viewport_camera,
                .cursors = &this->cursors,
            });
    }
}

auto Editor::handle_mouse_down(const SDL_MouseButtonEvent &event) -> void {
    glm::vec2 mouse_ndc_pos = get_mouse_ndc_coords();
    this->current_tool->handle_mouse_button_event(
        EditorTool::MouseButtonEventBundle{
            .event = &event,
            .mouse_ndc_coords = mouse_ndc_pos,
            .scene = this->scene.get(),
            .camera = &this->viewport_camera,
            .cursors = &this->cursors,
        });
}

auto Editor::handle_mouse_up(const SDL_MouseButtonEvent &event) -> void {
    glm::vec2 mouse_ndc_pos = get_mouse_ndc_coords();
    this->current_tool->handle_mouse_button_event(
        EditorTool::MouseButtonEventBundle{
            .event = &event,
            .mouse_ndc_coords = mouse_ndc_pos,
            .scene = this->scene.get(),
            .camera = &this->viewport_camera,
            .cursors = &this->cursors,
        });
}

auto Editor::new_empty_scene() -> void {
    // create new scene object
    this->scene = std::make_unique<vxng::scene::Scene>(SCENE_RESOLUTION,
                                                       DEFAULT_SCENE_SCALE);
    this->scene->init_webgpu(this->wgpu.device);

    this->renderer.set_scene(this->scene.get());
}

auto Editor::get_mouse_ndc_coords() const -> glm::vec2 {
    glm::vec2 screen_pos;
    SDL_GetMouseState(&screen_pos.x, &screen_pos.y);
    glm::ivec2 screen_size;
    SDL_GetWindowSize(this->sdl_window, &screen_size.x, &screen_size.y);

    // scale down to [0, 1]
    screen_pos /= screen_size;

    // flip y and scale both to [-1, 1]
    return (screen_pos - glm::vec2(0.5)) * glm::vec2(2.f, -2.f);
}
