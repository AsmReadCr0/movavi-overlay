#include <thread>
#include <overlay/overlay.h>

#include <main/actor/loop.h>

#include <utils/math/vector.h>
#include <utils/encryption/callstack.h>
#include <utils/encryption/xor.h>
#include <utils/var.hpp>


bool overlay::init( )
{
	hide;
	window = FindWindowA( enc( "Qt5152QWindowToolSaveBits" ).decrypt( ), enc( "Screen Recorder" ).decrypt() );
	if ( !window )
		return false;

	MARGINS window_margin = { -1 };
	hide_call( DwmExtendFrameIntoClientArea )( window, &window_margin );

	SetWindowPos( window, 0, 0, 0, monitor.width, monitor.height, SWP_SHOWWINDOW );

	hide_call( UpdateWindow )( window );
	hide_call( ShowWindow )( window, SW_SHOW );

	return true;
}

void overlay::render( )
{
	ImVec4 clear_color = ImColor( 0, 0, 0, 0 );

	ImGui_ImplDX11_NewFrame( );
	ImGui_ImplWin32_NewFrame( );
	ImGui::NewFrame( );

	actor::loop( );

	if ( config->misc.show_menu )
	{
		ImGui::GetForegroundDrawList( )->AddCircleFilled( ImGui::GetMousePos( ), 5, ImColor( 0, 247, 255 ) );
	}

	if ( config->misc.show_menu )
	{
		// render a menu
	}

	ImGui::Render( );

	const float clear_color_with_alpha[ 4 ] = {
		clear_color.x * clear_color.w,
		clear_color.y * clear_color.w,
		clear_color.z * clear_color.w,
		clear_color.w
	};

	d3d_device_ctx->OMSetRenderTargets( 1, &d3d_render_target, nullptr );
	d3d_device_ctx->ClearRenderTargetView( d3d_render_target, clear_color_with_alpha );

	ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );

	d3d_swap_chain->Present( config->misc.vsync ? 1 : 0, 0 );
}

bool overlay::render_loop( )
{
	static RECT rect_og;
	MSG msg = { NULL };
	ZeroMemory( &msg, sizeof( MSG ) );

	while ( msg.message != WM_QUIT )
	{
		UpdateWindow( window );
		ShowWindow( window, SW_SHOW );

		if ( ( PeekMessageA )( &msg, window, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		ImGuiIO& io = ImGui::GetIO( );

		io.DeltaTime = 1.0f / 60.0f;

		POINT p_cursor;
		GetCursorPos( &p_cursor );
		io.MousePos.x = p_cursor.x;
		io.MousePos.y = p_cursor.y;

		if ( GetAsyncKeyState( VK_LBUTTON ) )
		{
			io.MouseDown[ 0 ] = true;
			io.MouseClicked[ 0 ] = true;
			io.MouseClickedPos[ 0 ].x = io.MousePos.x;
			io.MouseClickedPos[ 0 ].x = io.MousePos.y;
		}
		else
			io.MouseDown[ 0 ] = false;

		overlay::render( );

	}

	ImGui_ImplDX11_Shutdown( );
	ImGui_ImplWin32_Shutdown( );
	ImGui::DestroyContext( );

	DestroyWindow( window );

	return true;
}

bool overlay::init_dx11( )
{
	DXGI_SWAP_CHAIN_DESC swap_chain_description;
	ZeroMemory( &swap_chain_description, sizeof( swap_chain_description ) );
	swap_chain_description.BufferCount = 2;
	swap_chain_description.BufferDesc.Width = 0;
	swap_chain_description.BufferDesc.Height = 0;
	swap_chain_description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_description.BufferDesc.RefreshRate.Numerator = 60;
	swap_chain_description.BufferDesc.RefreshRate.Denominator = 1;
	swap_chain_description.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swap_chain_description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_description.OutputWindow = window;
	swap_chain_description.SampleDesc.Count = 1;
	swap_chain_description.SampleDesc.Quality = 0;
	swap_chain_description.Windowed = 1;
	swap_chain_description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D_FEATURE_LEVEL d3d_feature_lvl;

	const D3D_FEATURE_LEVEL d3d_feature_array[ 2 ] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

	D3D11CreateDeviceAndSwapChain( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, d3d_feature_array, 2, D3D11_SDK_VERSION, &swap_chain_description, &d3d_swap_chain, &d3d_device, &d3d_feature_lvl, &d3d_device_ctx );

	ID3D11Texture2D* pBackBuffer;

	d3d_swap_chain->GetBuffer( 0, IID_PPV_ARGS( &pBackBuffer ) );

	d3d_device->CreateRenderTargetView( pBackBuffer, NULL, &d3d_render_target );

	pBackBuffer->Release( );

	ImGui::CreateContext( );

	ImGuiIO& io = ImGui::GetIO( ); ( void )io;

	ImFontConfig font_config;
	font_config.PixelSnapH = false;
	font_config.OversampleH = 5;
	font_config.OversampleV = 5;
	font_config.RasterizerMultiply = 1.2f;

	// add your preferred font
	//io.Fonts->AddFontFromMemoryTTF( , sizeof(  ), 17.0f, nullptr, io.Fonts->GetGlyphRangesDefault( ) );

	ImGui_ImplWin32_Init( window );
	ImGui_ImplDX11_Init( d3d_device, d3d_device_ctx );

	d3d_device->Release( );

	return true;
}