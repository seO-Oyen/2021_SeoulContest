#include "DXUT.h"
#include "resource.h"

#include <vector> //stl vector => 내가 길을 만들어가는 궤적을 저장하기 위해 사용

using namespace std;

vector<D3DXVECTOR2> trackPlayerPositions; //trackPlayerPositions은 실제로 vector2라는 값들을 연속적으로 들고 있을 것이다.

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
#define MAP_PROPERTY_VISITING 300
//#define MAP_PROPERTY_TEMP 500

enum PlayerState
{
    ON_EDGE, //현재 내 캐릭터의 상태가 edge위에 있다.
    GENERATING, //내가 만들고 있는 중이다.
};

PlayerState playerState = ON_EDGE; //처음에 플레이어 상태는 무조건 edge위에 있다.

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

void UpdateTextureMap() //현재 맵의 정보를 얻어 visit이면 땅을 먹은거 마냥 칠해주기
{
    RECT tdr = { 0, 0, 640, 480 };
    D3DLOCKED_RECT tlr;
    if (SUCCEEDED((*maskTex)->LockRect(0, &tlr, &tdr, 0)))
    {
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

    //내부
    for (int y = 200; y <= 300; ++y)
    {
        for (int x = 100; x <= 200; ++x)
        {
            map[y * 640 + x] = MAP_PROPERTY_VISIT; //외각선 안쪽 땅들을 전부 visit으로 한다.
        }
    }

    //외각선
    //y는 같고 x는 다르다 (세로선)
    /*for (int y = 200; y < 200 + 100; ++y) //직선의 값-> 표현을 했지만 보이지는 않는다.
    {
        int x = 100;
        map[y * 640 + x] = MAP_PROPERTY_EDGE;
    }
    for (int y = 200; y <= 200 + 100; ++y)
    {
        int x = 200; //그래서 x다름
        map[y * 640 + x] = MAP_PROPERTY_EDGE;
    }*/
    for (int y = 200; y <= 200 + 100; ++y)
    {
        map[y * 640 + 100] = MAP_PROPERTY_EDGE;
        map[y * 640 + 200] = MAP_PROPERTY_EDGE;
    }

    //x는 같고 y는 다르다 (가로선)
    /*for (int x = 100; x < 100 + 100; ++x)
    {
        int y = 200;
        map[y * 640 + x] = MAP_PROPERTY_EDGE;
    }
    for (int x = 100; x < 100 + 100; ++x)
    {
        int y = 300;
        map[y * 640 + x] = MAP_PROPERTY_EDGE;
    }*/
    for (int x = 100; x <= 200; ++x)
    {
        map[200 * 640 + x] = MAP_PROPERTY_EDGE;
        map[300 * 640 + x] = MAP_PROPERTY_EDGE;
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

    UpdateTextureMap();

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
    if (playerState == ON_EDGE) //플레이어가 edge일때 실행되는 코드
    {
        int curMapValue = map[py * 640 + px];

        if ((GetAsyncKeyState(VK_LEFT) & 0x8000) != 0) //왼쪽 화살표키를 눌렀다면
        {
            int mapValue = map[py * 640 + px - 1];
            if (mapValue == MAP_PROPERTY_EDGE)
            {
                px -= 1;
            }

            //현재 있는곳이 edge인데 가려는 곳이 empty이면 맵 생성을 시작 하겠다.
            if (mapValue == MAP_PROPERTY_EMPTY && curMapValue == MAP_PROPERTY_EDGE)
            {
                //맵 생성 시작
                playerState = GENERATING;
            }

        }
        if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0) //오른쪽 화살표키를 눌렀다면
        {
            //px += 1;
            int mapValue = map[py * 640 + px + 1]; //플레이어가 현재있는 맵 위치
            if (mapValue == MAP_PROPERTY_EDGE) //플레이어 위치가 EDGE라면 움직여라
            {
                px += 1;
            }

            if (mapValue == MAP_PROPERTY_EMPTY && curMapValue == MAP_PROPERTY_EDGE)
            {
                //맵 생성 시작
                playerState = GENERATING;
            }
        }
        if ((GetAsyncKeyState(VK_UP) & 0x8000) != 0) //위쪽 화살표키를 눌렀다면
        {
            //py -= 1;
            int mapValue = map[(py - 1) * 640 + px];
            if (mapValue == MAP_PROPERTY_EDGE)
            {
                py -= 1;
            }

            if (mapValue == MAP_PROPERTY_EMPTY && curMapValue == MAP_PROPERTY_EDGE)
            {
                //맵 생성 시작
                playerState = GENERATING;
            }
        }
        if ((GetAsyncKeyState(VK_DOWN) & 0x8000) != 0) //아래쪽 화살표키를 눌렀다면
        {
            //py += 1;
            int mapValue = map[(py + 1) * 640 + px];
            if (mapValue == MAP_PROPERTY_EDGE)
            {
                py += 1;
            }

            if (mapValue == MAP_PROPERTY_EMPTY && curMapValue == MAP_PROPERTY_EDGE)
            {
                //맵 생성 시작
                playerState = GENERATING;
            }
        }
    }
    else if (playerState == GENERATING) //playerState가 GENERATING상태라면 이 코드를 실행
    {
        if ((GetAsyncKeyState(VK_LEFT) & 0x8000) != 0)
        {
            int mapValue = map[py * 640 + px - 1];
            if (mapValue == MAP_PROPERTY_EMPTY) //empty상태에서 움직이는 거니까
            {
                px -= 1;

                map[py * 640 + px] = MAP_PROPERTY_VISITING;
                trackPlayerPositions.push_back(D3DXVECTOR2(px, py)); //현재의 위치값을 계속 저장해줌
            }
        }
        if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0)
        {
            //px += 1;
            int mapValue = map[py * 640 + px + 1]; //플레이어가 현재있는 맵 위치
            if (mapValue == MAP_PROPERTY_EMPTY) //플레이어 위치가 EDGE라면 움직여라
            {
                px += 1;

                map[py * 640 + px] = MAP_PROPERTY_VISITING;
                trackPlayerPositions.push_back(D3DXVECTOR2(px, py));
            }
        }
        if ((GetAsyncKeyState(VK_UP) & 0x8000) != 0)
        {
            //py -= 1;
            int mapValue = map[(py - 1) * 640 + px];
            if (mapValue == MAP_PROPERTY_EMPTY)
            {
                py -= 1;

                map[py * 640 + px] = MAP_PROPERTY_VISITING;
                trackPlayerPositions.push_back(D3DXVECTOR2(px, py));
            }
        }
        if ((GetAsyncKeyState(VK_DOWN) & 0x8000) != 0)
        {
            //py += 1;
            int mapValue = map[(py + 1) * 640 + px];
            if (mapValue == MAP_PROPERTY_EMPTY)
            {
                py += 1;

                map[py * 640 + px] = MAP_PROPERTY_VISITING;
                trackPlayerPositions.push_back(D3DXVECTOR2(px, py));
            }
        }
    }
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

        for (int i = 0; i < trackPlayerPositions.size(); ++i)
        {
            D3DXVECTOR3 trackPos(trackPlayerPositions[i].x, trackPlayerPositions[i].y, 0);
            spr->Draw(*dotTex, nullptr, nullptr, &trackPos, D3DCOLOR_RGBA(0, 255, 0, 255));
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