
/*

    todo
      
      - firewall queue<void()> for async. description and blocked writes can be up to a 900ms
      - chec file size with debug #ifed out
      - top 500 icon
      - patch notes instead of update button
      - profile
      - delete some files

    stretch

      -- json exceptions https://json.nlohmann.me/features/macros/#json_skip_unsupported_compiler_check

*/

#include "pch.h"

// data
#include "components/Endpoint.h"
// managers
#ifdef _DEBUG
#include "core/Debug.h"
#endif
#include "core/Settings.h"
#include "core/Dashboard.h"

#include "core/Firewall.h"
#include "core/Update.h"
#include "core/Tunneling.h"

#include "util/watcher/window.h"

// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs


#pragma comment(lib, "d3d11.lib")

#include <SDKDDKVer.h> // https://stackoverflow.com/a/43832497

#include "imgui-docking/imgui_impl_win32.h"
#include "imgui-docking/imgui_impl_dx11.h"
#include <d3d11.h>

#include "App.h"

// other
#include "theme.h"
#include "images.h"



// global managers structure
// http://clarkkromenaker.com/post/cpp-manager-access/
// the best way is probably "Global Variables"
// but "Service Locator" is more fun


// Data
class unknown_exception : public std::exception {
    const char* what() const {
        return "unkown exception";
    }
};


// globals
// TODO is this needed?
ID3D11Device* g_pd3dDevice = nullptr; // for media in browser

HWND g_hwnd = nullptr;


//namespace global {

#ifdef _DEBUG
    std::unique_ptr<Debug> g_debug;
#endif

    // testing
    std::unique_ptr<std::vector<std::shared_ptr<Endpoint2>>> g_endpoints;
    std::unique_ptr<Firewall> g_firewall;
    std::unique_ptr<Dashboard> g_dashboard;
    std::unique_ptr<Updater> g_updater;
    std::unique_ptr<Settings> g_settings;
    std::unique_ptr<core::tunneling::Tunneling> g_tunneling;
    std::unique_ptr<util::watcher::window::WindowWatcher> g_window_watcher;
//}




std::unordered_map<std::string, ImageTexture> APP_TEXTURES = { };

// fonts
ImFont* font_title = nullptr;
ImFont* font_subtitle = nullptr;
ImFont* font_text = nullptr;

bool dashboard_open = true;

// non globals
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);



void loadAssets() {

#ifdef _DEBUG
    util::timer::Timer timer("loadAssets");
#endif

    static const std::vector<std::string> textures = {
        "icon_options.png",
        "icon_maple_leaf.png",
        "icon_chain_slash.png",
        "icon_bolt.png",
        "icon_skull.png",
        "icon_clock_undo.png",
        "icon_heart.png",
        "icon_outside_window.png",

        "icon_wifi_slash.png",
        "icon_wifi_poor.png",
        "icon_wifi_fair.png",
        "icon_wifi.png",

        "icon_allow.png",
        "icon_block.png",
        "icon_wall_fire.png",

        "icon_angle.png",

        "background_app.png",
        "background_diagonal.png",
    };

    for (std::string texture : textures)
        _add_texture(texture.substr(0, texture.find(".")), texture.substr(texture.find(".") + 1));

    std::cout << "<" << std::hex << std::this_thread::get_id() << "> loaded " << std::dec << textures.size() << " textures." << std::endl;
}


#ifndef _DEBUG

#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#endif 

// Main code
int main(int, char**)
{

#ifdef _DEBUG
    util::timer::Timer timer("start");
#endif

    setlocale(LC_ALL, "en_US.UTF-8");

    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 200, 200, 90, 90, nullptr, nullptr, wc.hInstance, nullptr);
    g_hwnd = hwnd;

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    } 

    // Show the window
    //::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::ShowWindow(hwnd, SW_HIDE);


    // https://github.com/search?q=repo%3Avinjn%2FGpuProf%20IDI_ICON&type=code
    {
        HICON hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(101));

        // data pointer
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);
    }

    // hide debug console
    //::ShowWindow(::GetConsoleWindow(), SW_HIDE);
    //::ShowWindow(::GetConsoleWindow(), SW_NORMAL);

    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    io.ConfigDragClickToInputText = true;
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    //io.ConfigViewportsNoDefaultParent = true;
    //io.ConfigDockingAlwaysTabBar = true;
    //io.ConfigDockingTransparentPayload = true;
    //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: Experimental. THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
    //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI: Experimental.


    // theme
    setTheme(THEME::light);


    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    /*
        NOTE. i modified the dx11 imgui backend to support transparent backgrounds and rounded windows
            @ https://github.com/ocornut/imgui/issues/5988#issuecomment-1368490745
    */
    //ImGui_ImplWin32_EnableAlphaCompositing()

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // https://github.com/ocornut/imgui/blob/master/docs/FONTS.md#loading-font-data-from-memory
    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;

    // config regular
    {
        /*ImFontConfig config;
        config.OversampleH = 2;
        config.OversampleV = 1;
        config.GlyphExtraSpacing.x = 1.0f;*/

        //HRSRC resource = FindResource(NULL, L"CONFIG_REGULAR_2", L"OTF");
        HRSRC resource = FindResourceA(NULL, "ROBOTO_REGULAR", "OTF");
        //auto const size_pixels = 18;
        auto const size_pixels = 24;

        DWORD size = ::SizeofResource(NULL, resource);
        HGLOBAL data_handle = ::LoadResource(NULL, resource);

        // data pointer
        unsigned char* data = (unsigned char*) ::LockResource(data_handle);

        //font_config_regular_2 = io.Fonts->AddFontFromMemoryTTF(data, size, size_pixels);
        font_text = io.Fonts->AddFontFromMemoryTTF(data, size, size_pixels, &font_cfg);

        ::FreeResource(resource);
    }

    //ImGui::SetCurrentFont(font_text);

    // industry bold
    {

        HRSRC resource = FindResourceA(NULL, "INDUSTRY_BOLD", "OTF");
        //auto const size_pixels = 28;
        auto const size_pixels = 38;

        DWORD size = ::SizeofResource(NULL, resource);
        HGLOBAL data_handle = ::LoadResource(NULL, resource);

        // data pointer
        unsigned char* data = (unsigned char*) ::LockResource(data_handle);

        //font_industry_bold = io.Fonts->AddFontFromMemoryTTF(data, size, size_pixels);
        font_title = io.Fonts->AddFontFromMemoryTTF(data, size, size_pixels, &font_cfg);


        // TODO
        ::FreeResource(resource);
    }

    // industry medium
    {
        HRSRC resource = FindResourceA(NULL, "INDUSTRY_MEDIUM", "OTF");
        //HRSRC resource = FindResource(NULL, L"CONFIG_MEDIUM_2", L"OTF");
        auto const size_pixels = 20;

        DWORD size = ::SizeofResource(NULL, resource);
        HGLOBAL data_handle = ::LoadResource(NULL, resource);

        // data pointer
        unsigned char* data = (unsigned char*) ::LockResource(data_handle);

        font_subtitle = io.Fonts->AddFontFromMemoryTTF(data, size, size_pixels, &font_cfg);

        // TODO
        ::FreeResource(resource);
    }

    //io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 24, nullptr, io.Fonts->GetGlyphRangesJapanese());


    // https://github.com/ocornut/imgui/issues/5169

    // TODO /tmp
    io.IniFilename = NULL;
    /*//{
        std::filesystem::path path = std::filesystem::temp_directory_path().append("dropship");
        if (!std::filesystem::is_directory(path) || !std::filesystem::exists(path)) {
            std::filesystem::create_directory(path);
        }

        path.append("dropship.ini");

        std::string path_name = path.string();

        io.IniFilename = path_name.c_str();
    //}*/

    // ImGui::SaveIniSettingsToDisk(io.IniFilename);
    // printf("file path: %s\n", io.IniFilename);

    io.LogFilename = NULL;
    // store my own data :D
    //io.UserData = NULL

    static const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    /* TODO

        <big checklist here of ~9 things. preflight.>

        in UI:

            (good) firewall profiles validated
            (good) not on a vpn
            (warn) overwatch is open
            (warh) new app version available


    */

    std::optional<std::exception> error = std::nullopt;

    std::optional<std::future<void>> assets_future = std::nullopt;

    try {

        // NOTE make sure these are .reset() after done. otherwise process will exist after close.
#ifdef _DEBUG
        g_debug = std::make_unique<Debug>();
#endif
        g_endpoints = std::make_unique<std::vector<std::shared_ptr<Endpoint2>>>();
        g_firewall = std::make_unique<Firewall>();
        //g_process_watcher = std::make_unique<ProcessWatcher>("Overwatch.exe");
        g_window_watcher = std::make_unique<util::watcher::window::WindowWatcher>("Overwatch");
        g_settings = std::make_unique<Settings>();
        g_tunneling = std::make_unique<core::tunneling::Tunneling>();
        g_dashboard = std::make_unique<Dashboard>();
        g_updater = std::make_unique<Updater>();


        //loadAssets();
        assets_future = std::async(std::launch::async, loadAssets);
    }
    catch (const json::exception e)
    {
        //error = std::make_optional<json::exception>(e);
        error = std::make_optional<std::runtime_error>(e.what());
    }
    catch (const std::exception& e)
    {
        error = std::make_optional<std::exception>(e);
    }
    catch (...)
    {
        error = std::make_optional<unknown_exception>();
    }


#ifdef _DEBUG
    timer.stop();
#endif

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (error) {

            App::renderError(error.value().what());
        }
        else {
            try {

                if (dashboard_open)
                {
                    App::render(&dashboard_open);
                }

                else
                {
                    done = true;
                }
            }
            catch (const json::exception& e)
            {
                //error = std::make_optional<json::exception>(e);
                error = std::make_optional<std::runtime_error>(e.what());
            }
            catch (const std::exception& e)
            {
                error = e;
            }
            catch (...)
            {
                error = std::make_optional<unknown_exception>();
            }
        }

        /* testing */
        ImGui::ErrorCheckEndFrameRecover(nullptr);
        ImGui::ErrorCheckEndWindowRecover(nullptr);

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync

        {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup))
                    // TODO fix this?
                    //ImGui::CloseCurrentPopup();
                    //if (!ImGui::IsPopupOpen("updating")) {
                        ImGui::ClosePopupsOverWindow(ImGui::FindWindowByName("dashboard"), false);
                    //}
                else
                    done = true;
            }
        }
    }

    if (assets_future) {
        assets_future.value().get();
    }

#ifdef _DEBUG
    g_debug.reset();
#endif
    g_endpoints.reset();
    g_firewall.reset();
    g_window_watcher.reset();
    g_settings.reset();
    g_dashboard.reset();
    g_updater.reset();

    /*delete font_title;
    delete font_subtitle;
    delete font_text;*/


    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 20;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
        {
            //const int dpi = HIWORD(wParam);
            //printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
