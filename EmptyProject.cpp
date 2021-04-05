#include "DXUT.h"
#include "resource.h"

#include <vector> //stl vector => ���� ���� ������ ������ �����ϱ� ���� ���
#include <stack>

using namespace std;

vector<D3DXVECTOR2> trackPlayerPositions; //trackPlayerPositions�� ������ vector2��� ������ ���������� ��� ���� ���̴�.

LPDIRECT3DTEXTURE9* backgroundTex = nullptr; //BackgroundTex ���� �̹����� �޸� ���·� ��� �ִ�.
LPDIRECT3DTEXTURE9* maskTex = nullptr;
LPDIRECT3DTEXTURE9* dotTex = nullptr;
LPDIRECT3DTEXTURE9* playerTex = nullptr;
LPDIRECT3DTEXTURE9* alphaTex = nullptr;
LPD3DXSPRITE spr;

DWORD pixelData[640 * 480]; //��� 1�ް��� �ѱ� ������ �Լ� ���� �ƴ� �ٱ��� ���ش�

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

//�Ӽ�
#define EMPTY 0 //define = ����
#define VISIT 100
#define EDGE 200
#define VISITING 300
#define TEMP 500

enum PlayerState
{
    ON_EDGE, //���� �� ĳ������ ���°� edge���� �ִ�.
    GENERATING //���� ����� �ִ� ���̴�.
};

enum PlayerDirection
{
    EAST,
    WEST,
    SOUTH,
    NORTH,
};

PlayerState playerState = ON_EDGE; //ó���� �÷��̾� ���´� ������ edge���� �ִ�.
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
    for (int i = 0; i < 640 * 480; ++i) //�� ������ �׸��� �׸����̴�. 
    {
        map[i] = EMPTY; //map�� ���δ� empty�� �ʱ�ȭ �ϱ�
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

    dotTex = new LPDIRECT3DTEXTURE9(); //edge��� ǥ���� �κп� dot�� ǥ���Ұ��̴�
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

    playerTex = new LPDIRECT3DTEXTURE9(); //px, py���ٰ� �׸����̴�.
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

            // visit�̶�� ���� 8���� �ȼ��� ��� �ϳ��� visit�� �ƴ϶�� edge�� �Ǵ��Ѵ�.
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
    // ���� ��ȯ�� �ּ��� 3�ȼ� ���Ŀ� ����(������ ����� ���ؼ�)
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

            // trackPlayerPositions�� �ִ� ��ε��� �ٽ� Empty�� ���� ��Ų��.
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
                if (map[y * 640 + x] == EDGE) //���� ���� �Ӽ��� EDGE��� dot�� �׷���
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

        D3DXVECTOR3 playerPos(px - 3, py - 3, 0); //(100, 200)�� �÷��̾ ���ϰ��̴�.
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
    DXUTCreateWindow( L"�̸� ����" );
    DXUTCreateDevice( true, 640, 480 );

    // Start the render loop
    DXUTMainLoop();

    // TODO: Perform any application-level cleanup here

    return DXUTGetExitCode();
}