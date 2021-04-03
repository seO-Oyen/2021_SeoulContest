#include "DXUT.h"
#include "resource.h"


LPDIRECT3DTEXTURE9* backgroundTex = nullptr; //BackgroundTex 원본 이미지를 메모리 형태로 들고만 있다.
LPDIRECT3DTEXTURE9* maskTex = nullptr;
LPDIRECT3DTEXTURE9* dotTex = nullptr;
LPDIRECT3DTEXTURE9* playerTex = nullptr;
LPD3DXSPRITE spr;

DWORD pixelData[640 * 480]; //얘는 1메가가 넘기 때문에 함수 안이 아닌 바깥에 써준다

int map[640 * 480];

int px = 100; //playerX
int py = 200; //playerY

//속성
#define MAP_PROPERTY_EMPTY 0 //define = 정의
#define MAP_PROPERTY_VISIT 100
#define MAP_PROPERTY_EDGE 200
//#define MAP_PROPERTY_VISITING 300
//#define MAP_PROPERTY_TEMP 500

bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat,
                                      bool bWindowed, void* pUserContext )
{
    // Typically want to skip back buffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                                         AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
                                         D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;

    return true;
}


bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    return true;
}


HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
    for (int i = 0; i < 640 * 480; ++i) //이 정보로 그림을 그릴것이다. 
    {
        map[i] = MAP_PROPERTY_EMPTY; //map을 전부다 empty로 초기화 하기

        /*int x = i % 640;
        int y = i / 640;

        if (x >= 100 && x <= 200) //만약 x의 값이 100이상, 200이하라면 map의 속성을 visit으로 바꾸겠다.
        {
            map[i] = MAP_PROPERTY_VISIT;
        }*/
    }

    //y는 같고 x는 다르다 (세로선)
    for (int y = 200; y < 200 + 100; ++y) //직선의 값-> 표현을 했지만 보이지는 않는다.
    {
        int x = 100;
        map[y * 640 + x] = MAP_PROPERTY_EDGE;
    }
    for (int y = 200; y < 200 + 100; ++y)
    {
        int x = 200; //그래서 x다름
        map[y * 640 + x] = MAP_PROPERTY_EDGE;
    }

    //x는 같고 y는 다르다 (가로선)
    for (int x = 100; x < 100 + 100; ++x)
    {
        int y = 200;
        map[y * 640 + x] = MAP_PROPERTY_EDGE;
    }
    for (int x = 100; x < 100 + 100; ++x)
    {
        int y = 300;
        map[y * 640 + x] = MAP_PROPERTY_EDGE;
    }


    backgroundTex = new LPDIRECT3DTEXTURE9();
    D3DXCreateTextureFromFileExA(pd3dDevice,
        "background.png", D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0,
        0,
        D3DFMT_UNKNOWN,
        D3DPOOL_MANAGED,
        D3DX_DEFAULT,
        D3DX_DEFAULT,
        0,
        nullptr,
        nullptr, backgroundTex);


    maskTex = new LPDIRECT3DTEXTURE9();
    D3DXCreateTextureFromFileExA(pd3dDevice,
        "mask.png", D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0,
        0,
        D3DFMT_UNKNOWN,
        D3DPOOL_MANAGED,
        D3DX_DEFAULT,
        D3DX_DEFAULT,
        0,
        nullptr,
        nullptr, maskTex);

    dotTex = new LPDIRECT3DTEXTURE9(); //edge라고 표시한 부분에 dot을 표현할것이다
    D3DXCreateTextureFromFileExA(pd3dDevice,
        "dot.png", D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0,
        0,
        D3DFMT_UNKNOWN,
        D3DPOOL_MANAGED,
        D3DX_DEFAULT,
        D3DX_DEFAULT,
        0,
        nullptr,
        nullptr, dotTex);

    playerTex = new LPDIRECT3DTEXTURE9(); //px, py에다가 그릴것이다.
    D3DXCreateTextureFromFileExA(pd3dDevice,
        "player.png", D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0,
        0,
        D3DFMT_UNKNOWN,
        D3DPOOL_MANAGED,
        D3DX_DEFAULT,
        D3DX_DEFAULT,
        0,
        nullptr, 
        nullptr, playerTex);

    RECT tdr = { 0, 0, 640, 480 };
    D3DLOCKED_RECT tlr;
    if (SUCCEEDED((*backgroundTex)->LockRect(0, &tlr, &tdr, 0)))
    {
        DWORD* p = (DWORD*)tlr.pBits;
        memcpy(pixelData, p, 640 * 480 * 4);

        (*backgroundTex)->UnlockRect(0);
    }

    // 이미지를 읽은 후에 의도적으로 검정색 박스를 칠해보기.
    if (SUCCEEDED((*maskTex)->LockRect(0, &tlr, &tdr, 0)))
    {
        /*for (int y = 350; y < 350 + 100; ++y)
        {
            for (int x = 100; x < 100 + 200; ++x)
            {
                DWORD* p = (DWORD*)tlr.pBits;
                p[y * 640 + x] = pixelData[y * 640 + x]; //내가 먹은 영역만큼 비게
            }
        }*/

        //(0, 0, 200, 100) 땅을 먹은것 같은 효과주기
        /*for (int y = 0; y < 0 + 100; ++y)
        {
            for (int x = 0; x < 0 + 200; ++x)
            {
                DWORD* p = (DWORD*)tlr.pBits;
                p[y * 640 + x] = pixelData[y * 640 + x];
            }
        }*/


        //맵의 정보에 따라서 그림을 그린다. 
        for (int i = 0; i < 640 * 480; ++i)
        {
            //map[i] == MAP_PROPERTY_EMPTY //방문 안했을때는 아무것도 안할것이다.
            if (map[i] == MAP_PROPERTY_VISIT) //방문했을때는 원본그림이 그려졌음 좋겠다.
            {
                DWORD* p = (DWORD*)tlr.pBits;
                p[i] = pixelData[i];
            }
        }
                
        (*maskTex)->UnlockRect(0);
    }

    D3DXCreateSprite(pd3dDevice, &spr);
    return S_OK;
}

HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                    void* pUserContext )
{
    return S_OK;
}


void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
}


void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    HRESULT hr;

    // Clear the render target and the zbuffer 
    V( pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB( 0, 45, 50, 170 ), 1.0f, 0 ) );

    // Render the scene
    if( SUCCEEDED( pd3dDevice->BeginScene() ) )
    {
        spr->Begin(D3DXSPRITE_ALPHABLEND);

        spr->Draw(*backgroundTex, nullptr, nullptr, nullptr, D3DCOLOR_RGBA(255, 255, 255, 255));
        spr->Draw(*maskTex, nullptr, nullptr, nullptr, D3DCOLOR_RGBA(255, 255, 255, 255));

        for (int y = 0; y < 480; ++y)
        {
            for (int x = 0; x < 640; ++x)
            {
                if (map[y * 640 + x] == MAP_PROPERTY_EDGE) //만약 맵의 속성이 EDGE라면 dot을 그려라
                {
                    D3DXVECTOR3 edgePos(x, y, 0);
                    spr->Draw(*dotTex, nullptr, nullptr, &edgePos, D3DCOLOR_RGBA(0, 0, 0, 255));
                }
            }
        }

        D3DXVECTOR3 playerPos(px - 3, py - 3, 0); //(100, 200)에 플레이어가 보일것이다.
        spr->Draw(*playerTex, nullptr, nullptr, &playerPos, D3DCOLOR_RGBA(255, 255, 255, 255));

        spr->End();

        V( pd3dDevice->EndScene() );
    }
}


LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                          bool* pbNoFurtherProcessing, void* pUserContext )
{
    return 0;
}


void CALLBACK OnD3D9LostDevice( void* pUserContext )
{
}

void CALLBACK OnD3D9DestroyDevice( void* pUserContext )
{
    (*maskTex)->Release();
    (*backgroundTex)->Release();
    (*dotTex)->Release();
    (*playerTex)->Release();

    spr->Release();
}

INT WINAPI wWinMain( HINSTANCE, HINSTANCE, LPWSTR, int )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // Set the callback functions
    DXUTSetCallbackD3D9DeviceAcceptable( IsD3D9DeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( OnD3D9CreateDevice );
    DXUTSetCallbackD3D9DeviceReset( OnD3D9ResetDevice );
    DXUTSetCallbackD3D9FrameRender( OnD3D9FrameRender );
    DXUTSetCallbackD3D9DeviceLost( OnD3D9LostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnD3D9DestroyDevice );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackFrameMove( OnFrameMove );

    // TODO: Perform any application-level initialization here

    // Initialize DXUT and create the desired Win32 window and Direct3D device for the application
    DXUTInit( true, true ); // Parse the command line and show msgboxes
    DXUTSetHotkeyHandling( true, true, true );  // handle the default hotkeys
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"미림 게임" );
    DXUTCreateDevice( true, 640, 480 );

    // Start the render loop
    DXUTMainLoop();

    // TODO: Perform any application-level cleanup here

    return DXUTGetExitCode();
}