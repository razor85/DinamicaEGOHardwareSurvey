#include <algorithm>
#include <map>
#include <windows.h>
#include <winuser.h>  
#include <intrin.h>       
#include <iphlpapi.h>   

#include <iomanip>
#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <infoware/infoware.hpp>

#include "resource.h"
#include "curlPostInfo.h"

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define TEXT_BOX_ID 100
#define BUTTON_ID 101
#define LABEL_ID 102

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

HWND hWndEdit, hWndButton, hWndLabel;
HWND hWnd;
std::string hardwareInfo;


namespace {


std::string architecture_name( iware::cpu::architecture_t architecture ) noexcept {
  switch ( architecture ) {
  case iware::cpu::architecture_t::x64:
    return "x64";
  case iware::cpu::architecture_t::arm:
    return "ARM";
  case iware::cpu::architecture_t::itanium:
    return "Itanium";
  case iware::cpu::architecture_t::x86:
    return "x86";
  default:
    return "Unknown";
  }
}

const char* cache_type_name( iware::cpu::cache_type_t cache_type ) noexcept {
  switch ( cache_type ) {
  case iware::cpu::cache_type_t::unified:
    return "Unified";
  case iware::cpu::cache_type_t::instruction:
    return "Instruction";
  case iware::cpu::cache_type_t::data:
    return "Data";
  case iware::cpu::cache_type_t::trace:
    return "Trace";
  default:
    return "Unknown";
  }
}

std::string vendor_name( iware::gpu::vendor_t vendor ) noexcept {
  switch ( vendor ) {
  case iware::gpu::vendor_t::intel:
    return "Intel";
  case iware::gpu::vendor_t::amd:
    return "AMD";
  case iware::gpu::vendor_t::nvidia:
    return "NVidia";
  case iware::gpu::vendor_t::microsoft:
    return "Microsoft";
  case iware::gpu::vendor_t::qualcomm:
    return "Qualcomm";
  default:
    return "Unknown";
  }
}

int getVolumeHash() {
  DWORD serialNum = 0;

  // Determine if this volume uses an NTFS file system.      
  GetVolumeInformation( L"c:\\", NULL, 0, &serialNum, NULL, NULL, NULL, 0 );
  uint16_t hash = ( uint16_t ) ( ( serialNum + ( serialNum >> 16 ) ) & 0xFFFF );

  return hash;
}

int getCpuHash() {
  int cpuinfo[ 4 ] = { 0, 0, 0, 0 };
  __cpuid( cpuinfo, 0 );
  uint16_t hash = 0;
  uint16_t* ptr = ( uint16_t* ) ( &cpuinfo[ 0 ] );
  for ( uint32_t i = 0; i < 8; i++ )
    hash += ptr[ i ];

  return hash;
}

std::string getOSVersion() {
  const auto OS_info = iware::system::OS_info();
  std::stringstream OSVersion;
  OSVersion << OS_info.major << '.' << OS_info.minor << '.' << OS_info.patch << " build " << OS_info.build_number;
  return OSVersion.str();
}

std::string getHardwareJsonInformation() {
  const auto device_properties = iware::gpu::device_properties();
  const std::map< std::string, std::string > columns {
    { "ProcName", iware::cpu::model_name() },
    { "ProcArchitecture", architecture_name( iware::cpu::architecture() ) },
    { "ProcLogicalCPUs", std::to_string( iware::cpu::quantities().logical ) },
    { "ProcPhysicalCPUs", std::to_string( iware::cpu::quantities().physical ) },
    { "ProcCacheL1", std::to_string( iware::cpu::cache( 1 ).size ) },
    { "ProcCacheL2", std::to_string( iware::cpu::cache( 2 ).size ) },
    { "ProcCacheL3", std::to_string( iware::cpu::cache( 3 ).size ) },
    { "Memory", std::to_string( iware::system::memory().physical_total ) },
    { "OSName", iware::system::OS_info().full_name },
    { "OSVersion", getOSVersion() },
    { "GPU1Vendor", device_properties.size() > 0 ? vendor_name( device_properties[ 0 ].vendor ) : "" },
    { "GPU1Name", device_properties.size() > 0 ? device_properties[ 0 ].name : "" },
    { "GPU1VRAM", device_properties.size() > 0 ? std::to_string( device_properties[ 0 ].memory_size ) : "" },
    { "GPU2Vendor", device_properties.size() > 1 ? vendor_name( device_properties[ 1 ].vendor ) : "" },
    { "GPU2Name", device_properties.size() > 1 ? device_properties[ 1 ].name : "" },
    { "GPU2VRAM", device_properties.size() > 1 ? std::to_string( device_properties[ 1 ].memory_size ) : "" },
    { "GPU3Vendor", device_properties.size() > 2 ? vendor_name( device_properties[ 2 ].vendor ) : "" },
    { "GPU3Name", device_properties.size() > 2 ? device_properties[ 2 ].name : "" },
    { "GPU3VRAM", device_properties.size() > 2 ? std::to_string( device_properties[ 2 ].memory_size ) : "" },
    { "CPUSerialNumber", std::to_string( getCpuHash() ) },
    { "HardDriveNumber", std::to_string( getVolumeHash() ) }
  };
  std::stringstream buffer;
  buffer << "{";
  for ( auto column : columns ) {
    buffer << "\"" << column.first << "\":\"" << column.second << "\",";
  }
  std::string hardwareJson = buffer.str();
  hardwareJson.pop_back();
  hardwareJson.append( "}" );
  
  
  return hardwareJson;
}

std::wstring getHardwareInformation() {
  std::stringstream buffer;

  buffer
    << "Processor: " << iware::cpu::model_name() << '\n'
    << "  Architecture: " << architecture_name( iware::cpu::architecture() );

  const auto quantities = iware::cpu::quantities();
  buffer << "\n"
    << "  Logical CPUs: " << quantities.logical << '\n'
    << "  Physical CPUs: " << quantities.physical << '\n'
    << "  CPU packages: " << quantities.packages << '\n';

  for ( size_t i = 1; i <= 3; ++i ) {
    const auto cache = iware::cpu::cache( i );
    buffer << "  L" << i << ": " << cache.size << "B\n";
  }

  const auto memory = iware::system::memory();
  buffer << "\n" 
    << "Memory:\n"
    << "  Physical:\n"
    << "    Available: " << memory.physical_available << "B\n"
    << "    Total: " << memory.physical_total << "B\n"
    << "\n"
    << "  Virtual:\n"
    << "    Available: " << memory.virtual_available << "B\n"
    << "    Total: " << memory.virtual_total << "B\n";

  const auto OS_info = iware::system::OS_info();
  buffer << "\n"
    << "OS:\n"
    << "  Name: " << OS_info.name << '\n'
    << "  Full name: " << OS_info.full_name << '\n'
    << "  Version: " << OS_info.major << '.' << OS_info.minor << '.' 
    << OS_info.patch << " build " << OS_info.build_number << '\n';

  const auto device_properties = iware::gpu::device_properties();
  buffer << "\n"
    << "Graphics Processor:\n";

  if ( device_properties.empty() ) {
    buffer << "  Not detected.\n";
  } else {
    for ( auto i = 0u; i < device_properties.size(); ++i ) {
      const auto& properties_of_device = device_properties[ i ];
      buffer
        << "  Device #" << ( i + 1 ) << ":\n"
        << "    Vendor: " << vendor_name( properties_of_device.vendor ) << '\n'
        << "    Name: " << properties_of_device.name << '\n'
        << "    VRAM: " << properties_of_device.memory_size << "B\n"
        << "\n";
    }
  }

  const std::string bufferCopy = boost::algorithm::replace_all_copy( 
    buffer.str(), "\n", "\r\n" );

  return std::wstring( bufferCopy.begin(), bufferCopy.end() );
}


} // namespace ''

        
void callButtonIdEvent( const std::string& hardwareInfo ) {
  bool sentSuccessfully = CurlPost::sendPost( hardwareInfo );

  if ( sentSuccessfully ) {
    MessageBox( NULL, TEXT( "Hardware Survey complete, thanks!" ), 
      TEXT( "Success" ), MB_ICONINFORMATION );

    CloseWindow( hWnd );
    DestroyWindow( hWnd );

  } else { 

    MessageBox( NULL, TEXT( "Failed to send collected information, please try again" ), 
      TEXT( "Error" ), MB_ICONERROR );
  }
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
  switch ( msg ) {
  case WM_CTLCOLORSTATIC:
    {
      HDC hdc = ( HDC ) wParam;
      SetTextColor( hdc, RGB( 0, 0, 0 ) );
      SetBkMode( hdc, TRANSPARENT );
    }
    return ( LRESULT ) GetStockObject( DC_BRUSH );
  case WM_CTLCOLOREDIT:
    {
      HDC hdc = ( HDC ) wParam;
      SetTextColor( hdc, 0 );
    }
    return ( LRESULT ) GetStockObject( DC_BRUSH );
  case WM_ERASEBKGND:
    {
      RECT rc;
      GetClientRect( hWnd, &rc );
      SetBkColor( ( HDC ) wParam, GetSysColor( COLOR_WINDOW ) ); 
      ExtTextOut( ( HDC ) wParam, 0, 0, ETO_OPAQUE, &rc, 0, 0, 0 );
    }
    return 1;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      RECT rc;
      HDC hdc = BeginPaint( hWnd, &ps );
      GetClientRect( hWnd, &rc );
      SetBkColor( hdc, GetSysColor( COLOR_WINDOW ) );
      SetTextColor( hdc, 0x00000000 );
      ExtTextOut( hdc, 0, 0, ETO_OPAQUE, &rc, 0, 0, 0 );
      EndPaint( hWnd, &ps );
    }
    break;
  case WM_CREATE:
    {
      hWndLabel = CreateWindow( L"Static", 0,
                                WS_CHILD | WS_VISIBLE,
                                10, 5, 200, 15, hWnd, 
                                (HMENU) LABEL_ID, 
                                (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE), 
                                NULL );

      hWndEdit = CreateWindow( L"Edit", 0,
                               WS_CHILD | WS_BORDER | WS_VISIBLE | WS_VSCROLL | 
                               ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | 
                               ES_READONLY,
                               0, 0, 0, 0, hWnd, 
                               (HMENU) TEXT_BOX_ID, 
                               (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE), 
                               NULL );

      hWndButton = CreateWindow( L"Button", L"Submit",
                               WS_CHILD | WS_VISIBLE,
                               0, 0, 0, 0, hWnd, 
                               (HMENU) BUTTON_ID, 
                               (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE), 
                               NULL );

      HFONT hFont = CreateFont( 16, 0, 0, 0, FW_DONTCARE, 
                                FALSE, FALSE, FALSE, ANSI_CHARSET,
                                OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, 
                                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                                TEXT( "Segoe UI" ) );

      SendMessage( hWndEdit, WM_SETFONT, ( WPARAM ) hFont, TRUE );
      SendMessage( hWndButton, WM_SETFONT, ( WPARAM ) hFont, TRUE );
      SendMessage( hWndLabel, WM_SETFONT, ( WPARAM ) hFont, TRUE );

      const std::wstring hardwareInfo{ getHardwareInformation() };
      LPCWSTR infoPtr = hardwareInfo.c_str();

      SendMessage( hWndEdit, WM_SETTEXT, 0, ( LPARAM ) infoPtr );
      SendMessage( hWndLabel, WM_SETTEXT, 0, ( LPARAM ) 
        L"Survey Collected Information (Personal data will NOT be transfered):" );

      MoveWindow( hWnd,
                  GetSystemMetrics( SM_CXSCREEN ) / 2 - WINDOW_WIDTH / 2,
                  GetSystemMetrics( SM_CYSCREEN ) / 2 - WINDOW_HEIGHT / 2,
                  WINDOW_WIDTH,
                  WINDOW_HEIGHT,
                  TRUE );
    }
    return 0;
  case WM_SETFOCUS:
    SetFocus( hWndEdit );
    return 0;
  case WM_SIZE:
    {
      // Make the edit control the size of the window's client area. 
      MoveWindow( hWndEdit,
                  10, 25,
                  LOWORD( lParam ) - 20,
                  HIWORD( lParam ) - 70,
                  TRUE );

      MoveWindow( hWndButton,
                  LOWORD( lParam ) - 110, HIWORD( lParam ) - 35,
                  100, 20,
                  TRUE );
    }
    return 0;
  case WM_COMMAND:
    {
      const int wmId = LOWORD( wParam );
      const int wmEvent = HIWORD( wParam );

      switch ( wmId ) {
      case BUTTON_ID:
        const std::wstring hardwareInfo{ getHardwareInformation() };
        callButtonIdEvent( getHardwareJsonInformation() );
        break;
      }
    }
    return 0;
  case WM_DESTROY:
    PostQuitMessage( EXIT_SUCCESS );
    return 0;
  }

  return DefWindowProc( hWnd, msg, wParam, lParam );
}

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR nCmdLine, int nCmdShow ) {
  LPCTSTR windowClass = TEXT( "DinamicaEGOHardwareSurvey" );
  LPCTSTR windowTitle = TEXT( "Dinamica EGO Hardware Survey" );
  WNDCLASSEX wcex;

  wcex.cbClsExtra = 0;
  wcex.cbSize = sizeof( WNDCLASSEX );
  wcex.cbWndExtra = 0;
  wcex.hbrBackground = ( HBRUSH ) ( COLOR_WINDOW + 1 );
  wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
  wcex.hIcon = LoadIcon( NULL, IDI_APPLICATION );
  wcex.hIconSm = LoadIcon( NULL, IDI_APPLICATION );
  wcex.hInstance = hInstance;
  wcex.lpfnWndProc = WndProc;
  wcex.lpszClassName = windowClass;
  wcex.lpszMenuName = NULL;
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  if ( !RegisterClassEx( &wcex ) ) {
    MessageBox( NULL, TEXT( "RegisterClassEx Failed!" ), TEXT( "Error" ), MB_ICONERROR );
    return EXIT_FAILURE;
  }

  if ( !( hWnd = CreateWindowEx( 0, 
                                 windowClass, 
                                 windowTitle, 
                                 WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
                                 CW_USEDEFAULT, 
                                 CW_USEDEFAULT, 
                                 WINDOW_WIDTH, 
                                 WINDOW_HEIGHT, 
                                 NULL, NULL, hInstance, NULL ) ) ) {

    MessageBox( NULL, TEXT( "CreateWindow Failed!" ), TEXT( "Error" ), MB_ICONERROR );
    return EXIT_FAILURE;
  }

  HICON appIcon = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );

  SendMessageW( hWnd, WM_SETICON, ICON_BIG, ( LPARAM ) appIcon );
  ShowWindow( hWnd, nCmdShow );
  UpdateWindow( hWnd );

  MSG msg;
  while ( GetMessage( &msg, NULL, 0, 0 ) ) {
    TranslateMessage( &msg );
    DispatchMessage( &msg );
  }

  return EXIT_SUCCESS;
}
