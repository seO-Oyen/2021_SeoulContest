#include "DXUT.h"
#include "resource.h"

#include <vector> //stl vector => 내가 길을 만들어가는 궤적을 저장하기 위해 사용
#include <stack>

using namespace std;

vector<D3DXVECTOR2> trackPlayerPositions; //trackPlayerPositions은 실제로 vector2라는 값들을 연속적으로 들고 있을 것이다.

LPDIRECT3DTEXTURE9* backgroundTex = nullptr; //BackgroundTex 원본 이미지를 메모리 형태로 들고만 있다.
LPDIRECT3DTEXTURE9* maskTex = nullptr;
LPDIRECT3DTEXTURE9* dotTex = nullptr;
LPDIRECT3DTEXTURE9* playerTex = nullptr;
LPDIRECT3DTEXTURE9* alphaTex = nullptr;
LPD3DXSPRITE spr;

DWORD pixelData[640 * 480]; //얘는 1메가가 넘기 때문에 함수 안이 아닌 바깥에 써준다

DWORD* backTexValues;

int binaryMap[640 * 480];
int map[640 * 480];

int playerStartX = 300;
int playerStartY = 300;

int px = playerStartX;
int py = playerStartY;

bool isMoving = false;

int playerLimit = 5;
int playerDistance = 0;

//속성
#define EMPTY 0 //define = 정의
#define VISIT 100
#define EDGE 200
#define VISITING 300
#define TEMP 500

enum PlayerState
{
    ON_EDGE, //현재 내 캐릭터의 상태가 edge위에 있다.
    GENERATING //내가 만들고 있는 중이다.
};

enum PlayerDirection
{
    EAST,
    WEST,
    SOUTH,
    NORTH,
};

PlayerState playerState = ON_EDGE; //처음에 플레이어 상태는 무조건 edge위에 있다.
PlayerDirection playerDirection = EAST;

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
        map[i] = EMPTY; //map을 전부다 empty로 초기화 하기
    }

    for (int y = 0; y < 480; ++y)
    {
        map[y * 640 + 1] = EDGE;
        map[y * 640 + 638] = EDGE;
    }

    for (int x = 0; x < 640; ++x)
    {
        map[1 * 640 + x] = EDGE;
        map[478 * 640 + x] = EDGE;
    }

    px = 1;
    py = 478;

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

    alphaTex = new LPDIRECT3DTEXTURE9();
    D3DXCreateTextureFromFileExA(pd3dDevice,
        "alpha.png", D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0,
        0,
        D3DFMT_UNKNOWN,
        D3DPOOL_MANAGED,
        D3DX_DEFAULT,
        D3DX_DEFAULT,
        0,
        nullptr,
        nullptr, alphaTex);

    backTexValues = new DWORD[640 * 480];
    RECT tdr = { 0,0, 640, 480 };
    D3DLOCKED_RECT tlr;
    if (SUCCEEDED((*backgroundTex)->LockRect(0, &tlr, &tdr, 0)))
    {
        memcpy(backTexValues, (DWORD*)tlr.pBits, 640 * 480 * sizeof(DWORD));
        (*backgroundTex)->UnlockRect(0);
    }

    D3DXCreateSprite(pd3dDevice, &spr);
    return S_OK;
}

HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                    void* pUserContext )
{
    return S_OK;
}

struct Point
{
    int x;
    int y;
    Point(int _x, int _y)
        : x(_x), y(_y)
    {}
};

void Map_UpdateBorder()
{
    for (int y = 1; y < 480 - 1; ++y)
    {
        for (int x = 1; x < 640 - 1; ++x)
        {
            int binaryValue = binaryMap[y * 640 + x];
            if (binaryValue != VISIT) continue;

            // visit이라면 주위 8개의 픽셀을 얻고 하나라도 visit이 아니라면 edge로 판단한다.
            int left = binaryMap[y * 640 + (x - 1)];
            int top = binaryMap[(y - 1) * 640 + x];
            int right = binaryMap[y * 640 + (x + 1)];
            int leftTop = binaryMap[(y - 1) * 640 + (x - 1)];
            int rightTop = binaryMap[(y - 1) * 640 + (x + 1)];
            int leftBottom = binaryMap[(y + 1) * 640 + (x - 1)];
            int bottom = binaryMap[(y + 1) * 640 + x];
            int rightBottom = binaryMap[(y + 1) * 640 + (x + 1)];

            if (y == 0)
            {
                top = 0;
            }
            if (x == 0)
            {
                left = 0;
            }
            if (x == 640 - 1)
            {
                right = 0;
            }
            if (y == 480 - 1)
            {
                bottom = 0;
            }

            if (left != binaryValue ||
                top != binaryValue ||
                right != binaryValue ||
                leftTop != binaryValue ||
                rightTop != binaryValue ||
                leftBottom != binaryValue ||
                bottom != binaryValue ||
                rightBottom != binaryValue)
            {
                map[y * 640 + x] = EDGE;
            }
        }
    }
}

bool Map_CanVisit(int x, int y)
{
    if (x < 0) return false;
    if (y < 0) return false;
    if (x >= 640) return false;
    if (y >= 480) return false;

    return map[y * 640 + x] == EMPTY ||
        map[y * 640 + x] == VISIT ||
        map[y * 640 + x] == EDGE;
}

bool Map_IsEmpty(int x, int y)
{
    return map[y * 640 + x] == EMPTY;
}

bool Map_IsEdge(int x, int y)
{
    return map[y * 640 + x] == EDGE;
}

void floodFill(int x, int y, int s, int n)
{
    stack<int> floodStack;
    floodStack.push(y * 640 + x);

    while (!floodStack.empty())
    {
        int index = floodStack.top();
        floodStack.pop();

        int xIndex = index % 640;
        int yIndex = index / 640;

        if (xIndex < 0) continue;
        if (yIndex < 0) continue;
        if (xIndex >= 640) continue;
        if (yIndex >= 480) continue;
        if (map[index] != s) continue;

        map[index] = n;

        floodStack.push(yIndex * 640 + (xIndex - 1));
        floodStack.push(yIndex * 640 + (xIndex + 1));
        floodStack.push((yIndex - 1) * 640 + xIndex);
        floodStack.push((yIndex + 1) * 640 + xIndex);
    }
}

bool Map_SetProperty(int x, int y, int flag)
{
    if (map[y * 640 + x] == VISIT || map[y * 640 + x] == EDGE)
    {
        isMoving = false;
        playerState = ON_EDGE;

        floodFill(640 / 2, 480 / 2, EMPTY, TEMP);

        for (int i = 0; i < 640 * 480; ++i)
        {
            if (map[i] != TEMP)
            {
                binaryMap[i] = VISIT;
            }
            else
            {
                binaryMap[i] = EMPTY;
            }
        }

        memcpy(map, binaryMap, 640 * 480 * sizeof(int));

        RECT tdr = { 0,0, 640, 480 };
        D3DLOCKED_RECT tlr;
        if (SUCCEEDED((*alphaTex)->LockRect(0, &tlr, &tdr, 0)))
        {
            for (int i = 0; i < 640 * 480; ++i)
            {
                if (map[i] == VISIT)
                {
                    DWORD* p = (DWORD*)tlr.pBits;
                    p[i] = backTexValues[i];
                }
            }

            (*alphaTex)->UnlockRect(0);
        }

        Map_UpdateBorder();

        return false;
    }
    else
        map[y * 640 + x] = flag;

    return true;
}

void player_StartGenerate(PlayerDirection dir)
{
    // 방향 전환은 최소한 3픽셀 이후에 가능(영역을 만들기 위해서)
    playerDirection = dir;
    playerDistance = 0;

    isMoving = true;
    playerState = GENERATING;
    trackPlayerPositions.clear();

    playerStartX = px;
    playerStartY = py;
}

void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    if (playerState == ON_EDGE)
    {
        bool wantToGenerate = false;
        if ((GetAsyncKeyState(VK_CONTROL) & 0X8000) != 0)
        {
            wantToGenerate = true;
        }

        if ((GetAsyncKeyState(VK_LEFT) & 0X8000) != 0)
        {
            if (Map_IsEdge(px - 1, py))
            {
                px -= 1;
                isMoving = false;
                playerState = ON_EDGE;
            }
            else
            {
                if (Map_IsEmpty(px - 1, py) && wantToGenerate)
                {
                    player_StartGenerate(WEST);
                }
            }
        }
        if ((GetAsyncKeyState(VK_RIGHT) & 0X8000) != 0)
        {
            if (Map_IsEdge(px + 1, py))
            {
                px += 1;
                isMoving = false;
                playerState = ON_EDGE;
            }
            else
            {
                if (Map_IsEmpty(px + 1, py) && wantToGenerate)
                {
                    player_StartGenerate(EAST);
                }
            }
        }
        if ((GetAsyncKeyState(VK_UP) & 0X8000) != 0)
        {
            if (Map_IsEdge(px, py - 1))
            {
                py -= 1;
                isMoving = false;
                playerState = ON_EDGE;
            }
            else
            {
                if (Map_IsEmpty(px, py - 1) && wantToGenerate)
                {
                    player_StartGenerate(NORTH);
                }
            }
        }
        if ((GetAsyncKeyState(VK_DOWN) & 0X8000) != 0)
        {
            if (Map_IsEdge(px, py + 1))
            {
                py += 1;
                isMoving = false;
                playerState = ON_EDGE;
            }
            else
            {
                if (Map_IsEmpty(px, py + 1) && wantToGenerate)
                {
                    player_StartGenerate(SOUTH);
                }
            }
        }
    }
    else if (playerState == GENERATING)
    {
        if ((GetAsyncKeyState(VK_CONTROL) & 0X8000) == 0)
        {
            isMoving = false;
            playerState = ON_EDGE;

            // trackPlayerPositions에 있는 경로들은 다시 Empty로 복구 시킨다.
            for (int i = 0; i < trackPlayerPositions.size(); ++i)
            {
                const int index = trackPlayerPositions[i].y * 640 + trackPlayerPositions[i].x;
                map[index] = EMPTY;
            }
            trackPlayerPositions.clear();

            px = playerStartX;
            py = playerStartY;
        }

        if (isMoving)
        {
            if ((GetAsyncKeyState(VK_LEFT) & 0X8000) != 0)
            {
                if (Map_CanVisit(px - 1, py))
                {
                    if (playerDirection != WEST && playerDistance >= playerLimit)
                    {
                        playerDistance = 0;
                        playerDirection = WEST;
                    }

                    if (playerDirection == WEST)
                    {
                        playerDistance++;

                        px -= 1;

                        if (!Map_SetProperty(px, py, VISITING))
                        {
                            px += 1;
                        }
                        else
                        {
                            trackPlayerPositions.push_back(D3DXVECTOR2(px, py));
                        }
                    }
                }
            }
            else if ((GetAsyncKeyState(VK_RIGHT) & 0X8000) != 0)
            {
                if (Map_CanVisit(px + 1, py))
                {
                    if (playerDirection != EAST && playerDistance >= playerLimit)
                    {
                        playerDistance = 0;
                        playerDirection = EAST;
                    }

                    if (playerDirection == EAST)
                    {
                        playerDistance++;

                        px += 1;
                        if (!Map_SetProperty(px, py, VISITING))
                        {
                            px -= 1;
                        }
                        else
                        {
                            trackPlayerPositions.push_back(D3DXVECTOR2(px, py));
                        }
                    }
                }
            }
            else if ((GetAsyncKeyState(VK_UP) & 0X8000) != 0)
            {
                if (Map_CanVisit(px, py - 1))
                {
                    if (playerDirection != NORTH && playerDistance >= playerLimit)
                    {
                        playerDistance = 0;
                        playerDirection = NORTH;
                    }

                    if (playerDirection == NORTH)
                    {
                        playerDistance++;

                        py -= 1;
                        if (!Map_SetProperty(px, py, VISITING))
                        {
                            py += 1;
                        }
                        else
                        {
                            trackPlayerPositions.push_back(D3DXVECTOR2(px, py));
                        }
                    }
                }
            }
            else if ((GetAsyncKeyState(VK_DOWN) & 0X8000) != 0)
            {
                if (Map_CanVisit(px, py + 1))
                {
                    if (playerDirection != SOUTH && playerDistance >= playerLimit)
                    {
                        playerDistance = 0;
                        playerDirection = SOUTH;
                    }

                    if (playerDirection == SOUTH)
                    {
                        playerDistance++;

                        py += 1;
                        if (!Map_SetProperty(px, py, VISITING))
                        {
                            py -= 1;
                        }
                        else
                        {
                            trackPlayerPositions.push_back(D3DXVECTOR2(px, py));
                        }
                    }
                }
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
        spr->Draw(*maskTex, nullptr, nullptr, nullptr, D3DCOLOR_RGBA(255, 255, 255, 255));
        spr->End();

        spr->Begin(D3DXSPRITE_ALPHABLEND);
        spr->Draw(*alphaTex, nullptr, nullptr, nullptr, D3DCOLOR_RGBA(255, 255, 255, 255));        

        for (int y = 0; y < 480; ++y)
        {
            for (int x = 0; x < 640; ++x)
            {
                if (map[y * 640 + x] == EDGE) //만약 맵의 속성이 EDGE라면 dot을 그려라
                {
                    D3DXVECTOR3 edgePos(x, y, 0);
                    spr->Draw(*dotTex, nullptr, nullptr, &edgePos, D3DCOLOR_RGBA(0, 0, 0, 255));
                }

                if (map[y * 640 + x] == TEMP)
                {
                    D3DXVECTOR3 edgePos(x, y, 0);
                    spr->Draw(*dotTex, nullptr, nullptr, &edgePos, D3DCOLOR_RGBA(0, 0, 128, 255));
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
    (*alphaTex)->Release();

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