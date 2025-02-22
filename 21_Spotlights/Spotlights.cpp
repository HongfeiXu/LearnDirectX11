//--------------------------------------------------------------------------------------
// Include and link appropriate libraries and headers
//--------------------------------------------------------------------------------------
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "d3dx10.lib")
#pragma comment(lib, "DXErr.lib") // the dxerr.lib is no longer compatible with Visual Studio 2015 and later Visual Studio..
#pragma comment(lib, "legacy_stdio_definitions.lib") // so we link this, https://stackoverflow.com/questions/31053670/unresolved-external-symbol-vsnprintf-in-dxerr-lib
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#pragma comment (lib, "D3D10_1.lib")
#pragma comment (lib, "DXGI.lib")
#pragma comment (lib, "D2D1.lib")
#pragma comment (lib, "dwrite.lib")

#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include <DxErr.h>
#include <cassert>
#include <dinput.h>

#include <D3D10_1.h>
#include <DXGI.h>
#include <D2D1.h>
#include <sstream>
#include <dwrite.h>
#include <string>
#include <vector>

//--------------------------------------------------------------------------------------
// Macros
//--------------------------------------------------------------------------------------
#if defined(DEBUG) || defined(_DEBUG)
#define HR(hrr, description) \
if(FAILED(hrr))\
{\
	MessageBox(NULL, DXGetErrorDescription(hrr), LPCWSTR((std::to_wstring(__LINE__) + std::wstring(L" ") + TEXT(description)).c_str()), MB_OK); \
	return 0;\
}
#else
#define HR(hrr, description) ;
#endif

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct CBPerObject
{
	XMMATRIX WVP;
	XMMATRIX World; // 在世界空间计算光照效果
	XMMATRIX WorldInvTranspose;	// World 的逆转置矩阵, 用于法线变换
};

struct DirectionalLight
{
	DirectionalLight()
	{
		ZeroMemory(this, sizeof(DirectionalLight));
	}
	XMFLOAT3 dir;
	float pad;
	XMFLOAT4 ambient;
	XMFLOAT4 diffuse;
	XMFLOAT4 specular;
};

struct SpotLight
{
	SpotLight()
	{
		ZeroMemory(this, sizeof(SpotLight));
	}
	XMFLOAT3 pos;
	float innerCutOff;	// 内圆锥的余弦值
	XMFLOAT3 dir;		// 聚光灯所指向的方向
	float outerCutOff;	// 外圆锥的余弦值
	XMFLOAT3 att;		// 衰减
	float range;		// 最远照射距离

	XMFLOAT4 ambient;
	XMFLOAT4 diffuse;
	XMFLOAT4 specular;
};

struct CBPerFrame
{
	SpotLight light;
	XMFLOAT3 camWorldPos;
	float pad;
};

struct Vertex {
	Vertex() {}
	Vertex(float x, float y, float z,
		   float u, float v,
		   float nx, float ny, float nz)
		: pos(x, y, z), texCoord(u, v), normal(nx, ny, nz)
	{
	}

	XMFLOAT3 pos;
	XMFLOAT2 texCoord;
	XMFLOAT3 normal;
};


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
IDXGISwapChain*									g_pSwapChain;
ID3D11Device*									g_pd3d11Device;			// 用于检测显示适配器功能和分配资源
ID3D11DeviceContext*							g_pd3d11DevCon;			// 用于设置管线状态, 将资源绑定到图形管线和生成渲染命令
ID3D11RenderTargetView*							g_pRenderTargetView;	// 渲染目标视图(资源不能直接绑定到一个管线阶段, 必须为资源创建资源视图, 然后绑定到不同的管线阶段)
ID3D11DepthStencilView*							g_pDepthStencilView;
ID3D11Texture2D*								g_pDepthStencilBuffer;
ID3D11Buffer*									g_pCubeVertBuffer;
ID3D11Buffer*									g_pCubeIndexBuffer;
ID3D11Buffer*									g_pGroundVertBuffer;
ID3D11Buffer*									g_pGroundIndexBuffer;
ID3D11VertexShader*								g_pVS;
ID3D11PixelShader*								g_pPS;
ID3D10Blob*										g_pVS_Buffer;
ID3D10Blob*										g_pPS_Buffer;
ID3D11InputLayout*								g_pVertLayout;
ID3D11Buffer*									g_pCBPerObjectBuffer;
CBPerObject										g_CBPerObj;
ID3D11PixelShader*								g_pD2D_PS;
ID3D10Blob*										g_pD2D_PS_Buffer;
ID3D11Buffer*									g_pCBPerFrameBuffer;
CBPerFrame										g_CBPerFrame; // send this structure to the Pixel Shader constant buffers
SpotLight										g_light;
ID3D11ShaderResourceView*						g_pCubeTexture;
ID3D11SamplerState*								g_pLinearClampSS;
ID3D11ShaderResourceView*						g_pGroundTexture;
ID3D11SamplerState*								g_pLinearWrapSS;
ID3D11BlendState*								g_pTransparency; // for render text
ID3D11RasterizerState*							g_pCCWcullMode;	 // counter clockwise culling
ID3D11RasterizerState*							g_pCWcullMode;	 // clockwise culling
ID3D11RasterizerState*							g_pNoCullMode;	 // no culling
///////////////**************new**************////////////////////
ID3D11Buffer*									g_pSphereVertexBuffer;
ID3D11Buffer*									g_pSphereIndexBuffer;
ID3D11VertexShader*								g_pSkyboxVS;
ID3D11PixelShader*								g_pSkyboxPS;
ID3D10Blob*										g_pSkyboxVS_Buffer;
ID3D10Blob*										g_pSkyboxPS_Buffer;
ID3D11ShaderResourceView*						g_pSkyboxTexture;
//ID3D11SamplerState*							g_pSkyboxTexSamplerState;

ID3D11DepthStencilState*						DSLessEqual;	// make sure the skybox is always behind all other geometry in the world

int			NumSphereVertices;
int			NumSphereFaces;
XMMATRIX	g_WorldSphere;	// 用于Skybox的采样
XMMATRIX	g_WorldSphere2;	// 用于场景中的圆球
///////////////**************new**************////////////////////

///////////////**************new**************////////////////////
ID3D11VertexShader*								g_pReflectVS;
ID3D11PixelShader*								g_pReflectPS;
ID3D10Blob*										g_pReflectVS_Buffer;
ID3D10Blob*										g_pReflectPS_Buffer;
ID3D11SamplerState*								g_pAnistropicWrapSS; // 各向异性过滤模式
///////////////**************new**************////////////////////


// Simple Font
ID3D10Device1*									d3d101Device;
IDXGIKeyedMutex*								keyedMutex11;
IDXGIKeyedMutex*								keyedMutex10;
ID2D1RenderTarget*								D2DRenderTarget;
ID2D1SolidColorBrush*							Brush;
ID3D11Texture2D*								BackBuffer11;
ID3D11Texture2D*								sharedTex11;
ID3D11Buffer*									d2dVertBuffer;
ID3D11Buffer*									d2dIndexBuffer;
ID3D11ShaderResourceView*						d2dTexture;
IDWriteFactory*									DWriteFactory;
IDWriteTextFormat*								TextFormat;
std::wstring									printText;

// Input
IDirectInputDevice8*							DIKeyboard;
IDirectInputDevice8*							DIMouse;
DIMOUSESTATE									mouseLastState;
LPDIRECTINPUT8									DirectInput;

// Window
LPCTSTR						g_WndClassName = L"firstwindow";
HWND						g_hWnd = NULL;
HRESULT						g_hr;
const int					g_Width = 800;
const int					g_Height = 600;
bool						g_Enable4xMsaa = true;
float						g_rotx = 0;		// To rotate Cube1
float						g_rotz = 0;		// To rotate Cube1
float						g_Fov = 0.4f;	// To modify fov of camera

// Transform
XMMATRIX	 				g_WVP;
XMMATRIX	 				g_WorldCube1;
XMMATRIX	 				g_WorldCube2;
XMMATRIX	 				g_View;
XMMATRIX	 				g_Projection;
XMMATRIX	 				g_Rotation;		// UpdateScene()
XMMATRIX	 				g_Scale;		// UpdateScene()
XMMATRIX	 				g_Translation;	// UpdateScene()
float	 					g_Rot = 0.01f;	// keep track of rotation
XMMATRIX					g_RotationX;
XMMATRIX					g_RotationY;
XMMATRIX					g_RotationZ;

// Timer
double			countsPerSecond = 0.0;
__int64			CounterStart = 0;
int				frameCount = 0;
int				fps = 0;
__int64			frameTimeOld = 0;
double			frameTime;
bool			paused = false;

// FPS Camera
XMVECTOR	 	g_CamPosition;
XMVECTOR	 	g_CamTarget;
XMVECTOR	 	g_CamUp;
XMVECTOR		g_DefaultForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
XMVECTOR		g_DefaultRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
XMVECTOR		g_DefaultUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
XMVECTOR		g_CamForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
XMVECTOR		g_CamRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
XMMATRIX		g_CamRotationMatrix;
XMMATRIX		g_GroundWorld;
float			g_CamMoveLeftRight = 0.0f;
float			g_CamMoveBackForward = 0.0f;
float			g_CamPitch = 0.0f;	// 绕x轴旋转的角度
float			g_CamYaw = 0.0f;	// 绕y轴旋转的角度


//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
bool InitializeDirect3d11App(HINSTANCE hInstance);
void ReleaseObjects();
bool InitScene();
void DrawScene();
void UpdateScene(double time);
void RenderText(std::wstring text, int inInt);
void StartTimer();
double GetTime();
double GetFrameTime();
void PauseTimer();
bool InitializeWindow(HINSTANCE hInstance, int ShowWnd, int width, int height, bool windowed);
int messageloop();
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool InitD2D_D3D101_DWrite(IDXGIAdapter1 *Adapter);
bool InitD2DScreenTexture();
bool InitDirectInput(HINSTANCE hInstance);
void DetectInput(double time);
void UpdateCamera();
///////////////**************new**************////////////////////
void CreateSphere(int LatLines, int LongLines);
///////////////**************new**************////////////////////


//--------------------------------------------------------------------------------------
// Entry point to the program
//--------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance,	//Main windows function
				   HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine,
				   int nShowCmd)
{
	if(!InitializeWindow(hInstance, nShowCmd, g_Width, g_Height, true))
	{
		MessageBox(0, L"Window Initialization - Failed",
				   L"Error", MB_OK);
		return 0;
	}
	if(!InitializeDirect3d11App(hInstance))	//Initialize Direct3D
	{
		MessageBox(0, L"Direct3D Initialization - Failed",
				   L"Error", MB_OK);
		return 0;
	}
	if(!InitScene())	//Initialize our scene
	{
		MessageBox(0, L"Scene Initialization - Failed",
				   L"Error", MB_OK);
		return 0;
	}
	if(!InitDirectInput(hInstance))
	{
		MessageBox(0, L"Direct Input Initializetion - Failed",
				   L"Error", MB_OK);
		return 0;
	}

	messageloop();

	ReleaseObjects();

	return 0;
}

//--------------------------------------------------------------------------------------
// Initialize Window
//--------------------------------------------------------------------------------------
bool InitializeWindow(HINSTANCE hInstance,
					  int ShowWnd,
					  int width, int height,
					  bool windowed)
{
	typedef struct _WNDCLASS {
		UINT cbSize;
		UINT style;
		WNDPROC lpfnWndProc;
		int cbClsExtra;
		int cbWndExtra;
		HANDLE hInstance;
		HICON hIcon;
		HCURSOR hCursor;
		HBRUSH hbrBackground;
		LPCTSTR lpszMenuName;
		LPCTSTR lpszClassName;
	} WNDCLASS;

	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = g_WndClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if(!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Error registering class",
				   L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	g_hWnd = CreateWindowEx(
		NULL,
		g_WndClassName,
		L"FirstPersonCamera",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		width, height,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if(!g_hWnd)
	{
		MessageBox(NULL, L"Error creating window",
				   L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	ShowWindow(g_hWnd, ShowWnd);
	UpdateWindow(g_hWnd);

	return true;
}

//--------------------------------------------------------------------------------------
// Initialize Direct3D 11
//--------------------------------------------------------------------------------------
bool InitializeDirect3d11App(HINSTANCE hInstance)
{
	// Create DXGI factory to enumerate adapters
	// Use the first adapters
	IDXGIFactory1* dxgiFactory;
	g_hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)& dxgiFactory);
	HR(g_hr, "CreateDXGIFactory1");
	IDXGIAdapter1* adapter;
	g_hr = dxgiFactory->EnumAdapters1(0, &adapter);
	HR(g_hr, "dxgiFactory->EnumAdapters1");

	// Create Device and Context (+ DX11 龙书中译 P63)
	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel;
	g_hr = D3D11CreateDevice(
		adapter,					//***new***//
		D3D_DRIVER_TYPE_UNKNOWN,	//***new***//
		0,
		createDeviceFlags | D3D11_CREATE_DEVICE_BGRA_SUPPORT,	//***new***//
		0, 0,
		D3D11_SDK_VERSION,
		&g_pd3d11Device,
		&featureLevel,
		&g_pd3d11DevCon);
	HR(g_hr, "D3D11CreateDevice");
	if(featureLevel != D3D_FEATURE_LEVEL_11_0)
	{
		MessageBox(NULL, L"Direct3D FeatureLevel 11 unsupported.", 0, 0);
		return 0;
	}

	//Initialize Direct2D, Direct3D 10.1, DirectWrite
	InitD2D_D3D101_DWrite(adapter);

	//Release the Adapter interface
	adapter->Release();

	// Check MSAA (+ DX11 龙书中译 P65)
	UINT m4xMsaaQuality;
	g_pd3d11Device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMsaaQuality);
	assert(m4xMsaaQuality > 0);

	// Describe our backbuffer
	DXGI_MODE_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));
	bufferDesc.Width = g_Width;
	bufferDesc.Height = g_Height;
	bufferDesc.RefreshRate.Numerator = 60;
	bufferDesc.RefreshRate.Denominator = 1;
	bufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;			//***new***// D2D is only compatable with this format
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Describe our SwapChain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapChainDesc.BufferDesc = bufferDesc;
	// 是否使用 4X MSAA (+ DX11 龙书中译 P67)
	if(g_Enable4xMsaa)
	{
		swapChainDesc.SampleDesc.Count = 4;
		swapChainDesc.SampleDesc.Quality = m4xMsaaQuality - 1;
	}
	else
	{
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
	}
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = g_hWnd;
	///////////////**************new**************////////////////////
	swapChainDesc.Windowed = true;
	///////////////**************new**************////////////////////
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;

	// Create our SwapChain
	g_hr = dxgiFactory->CreateSwapChain(g_pd3d11Device, &swapChainDesc, &g_pSwapChain);
	HR(g_hr, "dxgiFactory->CreateSwapChain");
	dxgiFactory->Release();

	// Create Render Target View
	ID3D11Texture2D* backBuffer;
	g_hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)); // 获取一个交换链的后台缓冲区指针
	HR(g_hr, "g_pSwapChain->GetBuffer");

	g_hr = g_pd3d11Device->CreateRenderTargetView(backBuffer, 0, &g_pRenderTargetView); // 创建渲染目标视图
	HR(g_hr, "g_pd3d11Device->CreateRenderTargetView");
	backBuffer->Release(); // now we don't need the backbuffer anymore, we can release it

	// Describe our Depth/Stencil Buffer
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = g_Width;
	depthStencilDesc.Height = g_Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	if(g_Enable4xMsaa)
	{
		depthStencilDesc.SampleDesc.Count = 4;
		depthStencilDesc.SampleDesc.Quality = m4xMsaaQuality - 1;
	}
	else
	{
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
	}
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	// Create the Depth/Stencil View
	g_pd3d11Device->CreateTexture2D(&depthStencilDesc, 0, &g_pDepthStencilBuffer);
	g_pd3d11Device->CreateDepthStencilView(g_pDepthStencilBuffer, 0, &g_pDepthStencilView);

	// 将视图绑定到输出合并阶段
	g_pd3d11DevCon->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	return true;
}


//--------------------------------------------------------------------------------------
// Init Scene
//--------------------------------------------------------------------------------------
bool InitScene()
{
	if(!InitD2DScreenTexture())
		return false;

	// Define the Input Layout
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	UINT numElements = ARRAYSIZE(layout); // hold the size of our input layout array


	// Compile Shader from shader file
	g_hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "VS", "vs_5_0", 0, 0, 0, &g_pVS_Buffer, 0, 0);
	HR(g_hr, "D3DX11CompileFromFile");
	g_hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "PS", "ps_5_0", 0, 0, 0, &g_pPS_Buffer, 0, 0);
	HR(g_hr, "D3DX11CompileFromFile");
	g_hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "D2D_PS", "ps_5_0", 0, 0, 0, &g_pD2D_PS_Buffer, 0, 0);
	HR(g_hr, "D3DX11CompileFromFile");
	///////////////**************new**************////////////////////
	g_hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "SKYMAP_VS", "vs_5_0", 0, 0, 0, &g_pSkyboxVS_Buffer, 0, 0);
	HR(g_hr, "D3DX11CompileFromFile");
	g_hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "SKYMAP_PS", "ps_5_0", 0, 0, 0, &g_pSkyboxPS_Buffer, 0, 0);
	HR(g_hr, "D3DX11CompileFromFile");
	g_hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "REFLECT_VS", "vs_5_0", 0, 0, 0, &g_pReflectVS_Buffer, 0, 0);
	HR(g_hr, "D3DX11CompileFromFile");
	g_hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "REFLECT_PS", "ps_5_0", 0, 0, 0, &g_pReflectPS_Buffer, 0, 0);
	HR(g_hr, "D3DX11CompileFromFile");
	///////////////**************new**************////////////////////

	// Create the Shader Objects
	g_hr = g_pd3d11Device->CreateVertexShader(g_pVS_Buffer->GetBufferPointer(), g_pVS_Buffer->GetBufferSize(), NULL, &g_pVS);
	HR(g_hr, "g_pd3d11Device->CreateVertexShader");
	g_hr = g_pd3d11Device->CreatePixelShader(g_pPS_Buffer->GetBufferPointer(), g_pPS_Buffer->GetBufferSize(), NULL, &g_pPS);
	HR(g_hr, "g_pd3d11Device->CreatePixelShader");
	g_hr = g_pd3d11Device->CreatePixelShader(g_pD2D_PS_Buffer->GetBufferPointer(), g_pD2D_PS_Buffer->GetBufferSize(), NULL, &g_pD2D_PS);
	HR(g_hr, "g_pd3d11Device->CreatePixelShader");
	///////////////**************new**************////////////////////
	g_hr = g_pd3d11Device->CreateVertexShader(g_pSkyboxVS_Buffer->GetBufferPointer(), g_pSkyboxVS_Buffer->GetBufferSize(), NULL, &g_pSkyboxVS);
	HR(g_hr, "g_pd3d11Device->CreateVertexShader");
	g_hr = g_pd3d11Device->CreatePixelShader(g_pSkyboxPS_Buffer->GetBufferPointer(), g_pSkyboxPS_Buffer->GetBufferSize(), NULL, &g_pSkyboxPS);
	HR(g_hr, "g_pd3d11Device->CreatePixelShader");
	g_hr = g_pd3d11Device->CreateVertexShader(g_pReflectVS_Buffer->GetBufferPointer(), g_pReflectVS_Buffer->GetBufferSize(), NULL, &g_pReflectVS);
	HR(g_hr, "g_pd3d11Device->CreateVertexShader");
	g_hr = g_pd3d11Device->CreatePixelShader(g_pReflectPS_Buffer->GetBufferPointer(), g_pReflectPS_Buffer->GetBufferSize(), NULL, &g_pReflectPS);
	HR(g_hr, "g_pd3d11Device->CreatePixelShader");
	///////////////**************new**************////////////////////

	// Set Vertex and Pixel Shaders
	g_pd3d11DevCon->VSSetShader(g_pVS, 0, 0);
	g_pd3d11DevCon->PSSetShader(g_pPS, 0, 0);

	///////////////**************new**************////////////////////
	CreateSphere(20, 20);
	///////////////**************new**************////////////////////

	Vertex vertices[] =
	{
		// Front Face
		Vertex(-1.0f, -1.0f, -1.0f, 0.0f, 1.0f,-1.0f, -1.0f, -1.0f),
		Vertex(-1.0f,  1.0f, -1.0f, 0.0f, 0.0f,-1.0f,  1.0f, -1.0f),
		Vertex(1.0f,  1.0f, -1.0f, 1.0f, 0.0f, 1.0f,  1.0f, -1.0f),
		Vertex(1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f),

		// Back Face
		Vertex(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f,-1.0f, -1.0f, 1.0f),
		Vertex(1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f),
		Vertex(1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  1.0f, 1.0f),
		Vertex(-1.0f,  1.0f, 1.0f, 1.0f, 0.0f,-1.0f,  1.0f, 1.0f),

		// Top Face
		Vertex(-1.0f, 1.0f, -1.0f, 0.0f, 1.0f,-1.0f, 1.0f, -1.0f),
		Vertex(-1.0f, 1.0f,  1.0f, 0.0f, 0.0f,-1.0f, 1.0f,  1.0f),
		Vertex(1.0f, 1.0f,  1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f),
		Vertex(1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f),

		// Bottom Face
		Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f,-1.0f, -1.0f, -1.0f),
		Vertex(1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f),
		Vertex(1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, -1.0f,  1.0f),
		Vertex(-1.0f, -1.0f,  1.0f, 1.0f, 0.0f,-1.0f, -1.0f,  1.0f),

		// Left Face
		Vertex(-1.0f, -1.0f,  1.0f, 0.0f, 1.0f,-1.0f, -1.0f,  1.0f),
		Vertex(-1.0f,  1.0f,  1.0f, 0.0f, 0.0f,-1.0f,  1.0f,  1.0f),
		Vertex(-1.0f,  1.0f, -1.0f, 1.0f, 0.0f,-1.0f,  1.0f, -1.0f),
		Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f,-1.0f, -1.0f, -1.0f),

		// Right Face
		Vertex(1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f),
		Vertex(1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  1.0f, -1.0f),
		Vertex(1.0f,  1.0f,  1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  1.0f),
		Vertex(1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 1.0f, -1.0f,  1.0f),
	};

	DWORD indices[] = {
		// Front Face
		0,  1,  2,
		0,  2,  3,

		// Back Face
		4,  5,  6,
		4,  6,  7,

		// Top Face
		8,  9, 10,
		8, 10, 11,

		// Bottom Face
		12, 13, 14,
		12, 14, 15,

		// Left Face
		16, 17, 18,
		16, 18, 19,

		// Right Face
		20, 21, 22,
		20, 22, 23
	};

	Vertex groundVertices[] =
	{
		// Bottom Face
		Vertex(-1.0f, -1.0f, -1.0f, 10.0f, 10.0f, 0.0f, 1.0f, 0.0f),
		Vertex(1.0f, -1.0f, -1.0f,   0.0f, 10.0f, 0.0f, 1.0f, 0.0f),
		Vertex(1.0f, -1.0f,  1.0f,   0.0f,   0.0f, 0.0f, 1.0f, 0.0f),
		Vertex(-1.0f, -1.0f,  1.0f, 10.0f,   0.0f, 0.0f, 1.0f, 0.0f),
	};

	DWORD groundIndices[] =
	{
		0,  2,  1,
		0,  3,  2,
	};

	// Create the index buffer
	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

	indexBufferDesc.ByteWidth = sizeof(indices);
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexBufferData;
	ZeroMemory(&indexBufferData, sizeof(indexBufferData));

	indexBufferData.pSysMem = indices;
	g_hr = g_pd3d11Device->CreateBuffer(&indexBufferDesc, &indexBufferData, &g_pCubeIndexBuffer);
	HR(g_hr, "g_pd3d11Device->CreateBuffer");

	indexBufferDesc.ByteWidth = sizeof(groundIndices);
	indexBufferData.pSysMem = groundIndices;
	g_hr = g_pd3d11Device->CreateBuffer(&indexBufferDesc, &indexBufferData, &g_pGroundIndexBuffer);
	HR(g_hr, "g_pd3d11Device->CreateBuffer");

	// Create the vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

	vertexBufferDesc.ByteWidth = sizeof(vertices);
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;
	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));

	vertexBufferData.pSysMem = vertices;
	g_hr = g_pd3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &g_pCubeVertBuffer);
	HR(g_hr, "g_pd3d11Device->CreateBuffer");

	vertexBufferDesc.ByteWidth = sizeof(groundVertices);
	vertexBufferData.pSysMem = groundVertices;
	g_hr = g_pd3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &g_pGroundVertBuffer);;
	HR(g_hr, "g_pd3d11Device->CreateBuffer");

	// Create the Input Layout
	g_hr = g_pd3d11Device->CreateInputLayout(layout, numElements, g_pVS_Buffer->GetBufferPointer(),
											 g_pVS_Buffer->GetBufferSize(), &g_pVertLayout);
	HR(g_hr, "g_pd3d11Device->CreateInputLayout");

	// Set the Input Layout
	g_pd3d11DevCon->IASetInputLayout(g_pVertLayout);

	// Set Primitive Topology
	g_pd3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the Viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(viewport));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = g_Width;
	viewport.Height = g_Height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Set the Viewport
	g_pd3d11DevCon->RSSetViewports(1, &viewport);

	// Create buffers to send to the cbuffers in effect file
	D3D11_BUFFER_DESC cbbd;
	ZeroMemory(&cbbd, sizeof(cbbd));
	cbbd.ByteWidth = sizeof(CBPerObject);
	cbbd.Usage = D3D11_USAGE_DEFAULT;
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;
	cbbd.StructureByteStride = 0;
	g_hr = g_pd3d11Device->CreateBuffer(&cbbd, 0, &g_pCBPerObjectBuffer);
	HR(g_hr, "g_pd3d11Device->CreateBuffer");

	ZeroMemory(&cbbd, sizeof(cbbd));
	cbbd.ByteWidth = sizeof(CBPerFrame);
	cbbd.Usage = D3D11_USAGE_DEFAULT;
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;
	cbbd.StructureByteStride = 0;
	g_hr = g_pd3d11Device->CreateBuffer(&cbbd, 0, &g_pCBPerFrameBuffer);
	HR(g_hr, "g_pd3d11Device->CreateBuffer");

	// Load the Texture from a file
	g_hr = D3DX11CreateShaderResourceViewFromFile(g_pd3d11Device, L"braynzar.jpg", NULL, NULL, &g_pCubeTexture, NULL);
	HR(g_hr, "D3DX11CreateShaderResourceViewFromFile");
	g_hr = D3DX11CreateShaderResourceViewFromFile(g_pd3d11Device, L"grass.jpg", NULL, NULL, &g_pGroundTexture, NULL);
	HR(g_hr, "D3DX11CreateShaderResourceViewFromFile");

	///////////////**************new**************////////////////////
	// Load cube texture
	D3DX11_IMAGE_LOAD_INFO loadSMInfo;
	loadSMInfo.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE; // tell D3D we will be loading a texture cube
	
	ID3D11Texture2D* SMTexture = 0;
	g_hr = D3DX11CreateTextureFromFile(g_pd3d11Device, L"skybox.dds", NULL, NULL, (ID3D11Resource**)&SMTexture, NULL); // create a 2D texture from file
	HR(g_hr, "D3DX11CreateTextureFromFile");
	
	D3D11_TEXTURE2D_DESC SMTextureDesc;
	SMTexture->GetDesc(&SMTextureDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC SMViewDesc;
	SMViewDesc.Format = SMTextureDesc.Format;
	SMViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	SMViewDesc.TextureCube.MipLevels = SMTextureDesc.MipLevels;
	SMViewDesc.TextureCube.MostDetailedMip = 0;

	g_hr = g_pd3d11Device->CreateShaderResourceView(SMTexture, &SMViewDesc, &g_pSkyboxTexture);
	HR(g_hr, "g_pd3d11Device->CreateShaderResourceView");
	///////////////**************new**************////////////////////

	// Describe the Sample State
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create Sampler State
	g_hr = g_pd3d11Device->CreateSamplerState(&sampDesc, &g_pLinearClampSS);
	HR(g_hr, "g_pd3d11Device->CreateSamplerState");
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	g_hr = g_pd3d11Device->CreateSamplerState(&sampDesc, &g_pLinearWrapSS);
	HR(g_hr, "g_pd3d11Device->CreateSamplerState");
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.MaxAnisotropy = 4;
	g_hr = g_pd3d11Device->CreateSamplerState(&sampDesc, &g_pAnistropicWrapSS);
	HR(g_hr, "g_pd3d11Device->CreateSamplerState");


	// Define our blending equation and create it.
	D3D11_RENDER_TARGET_BLEND_DESC rtbd;
	ZeroMemory(&rtbd, sizeof(rtbd));
	rtbd.BlendEnable = true;
	rtbd.SrcBlend = D3D11_BLEND_SRC_COLOR;
	rtbd.DestBlend = D3D11_BLEND_BLEND_FACTOR;
	rtbd.BlendOp = D3D11_BLEND_OP_ADD;
	rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget[0] = rtbd;

	g_hr = g_pd3d11Device->CreateBlendState(&blendDesc, &g_pTransparency);
	HR(g_hr, "g_pd3d11Device->CreateBlendState");

	// Describe and Create the Rasterizer Render State
	D3D11_RASTERIZER_DESC cmdesc;
	ZeroMemory(&cmdesc, sizeof(D3D11_RASTERIZER_DESC));

	cmdesc.FillMode = D3D11_FILL_SOLID;
	//cmdesc.FillMode = D3D11_FILL_WIREFRAME;
	cmdesc.CullMode = D3D11_CULL_BACK;
	cmdesc.MultisampleEnable = TRUE; // 开启多重采样抗锯齿, 当 enable4xMsaa 也为 true 时有效.
	cmdesc.FrontCounterClockwise = true;
	g_hr = g_pd3d11Device->CreateRasterizerState(&cmdesc, &g_pCCWcullMode);
	HR(g_hr, "g_pd3d11Device->CreateRasterizerState");

	cmdesc.FrontCounterClockwise = false;
	g_hr = g_pd3d11Device->CreateRasterizerState(&cmdesc, &g_pCWcullMode);
	HR(g_hr, "g_pd3d11Device->CreateRasterizerState");

	cmdesc.CullMode = D3D11_CULL_NONE;
	g_hr = g_pd3d11Device->CreateRasterizerState(&cmdesc, &g_pNoCullMode);
	HR(g_hr, "g_pd3d11Device->CreateRasterizerState");

	///////////////**************new**************////////////////////
	// Describe and Create Depth/Stencil Render State
	// 允许使用深度值一致的像素进行替换深度/模板状态
	// 该状态用于绘制天空盒, 因为深度值为 1.0 时 默认无法通过深度测试
	D3D11_DEPTH_STENCIL_DESC dssDesc;
	ZeroMemory(&dssDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dssDesc.DepthEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dssDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	dssDesc.StencilEnable = false;
	g_hr = g_pd3d11Device->CreateDepthStencilState(&dssDesc, &DSLessEqual);
	HR(g_hr, "g_pd3d11Device->CreateDepthStencilState");
	///////////////**************new**************////////////////////

	// Define the light
	g_light.pos = XMFLOAT3(0.0f, 1.0f, -2.0f);
	g_light.dir = XMFLOAT3(0.0f, 0.0f, 1.0f);
	g_light.att = XMFLOAT3(1.0f, 0.09f, 0.0032f);
	g_light.innerCutOff = cos(12.5 / 180 * 3.14);
	g_light.outerCutOff = cos(17.5 / 180 * 3.14);
	g_light.range = 50.0f;
	g_light.ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	g_light.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	g_light.specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// Camera infomation(a simple static camera)
	g_CamPosition = XMVectorSet(0.0f, 3.0f, -8.0f, 0.0f);
	g_CamTarget = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	g_CamUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	// Set the View matrix
	g_View = XMMatrixLookAtLH(g_CamPosition, g_CamTarget, g_CamUp);

	// Set the Project matrix
	g_Projection = XMMatrixPerspectiveFovLH(g_Fov * 3.14f, (float)g_Width / g_Height, 1.0f, 1000.0f);

	return true;
}

//--------------------------------------------------------------------------------------
// Init direct input
//--------------------------------------------------------------------------------------
bool InitDirectInput(HINSTANCE hInstance)
{
	// create a Direct Input object
	g_hr = DirectInput8Create(hInstance,
							  DIRECTINPUT_VERSION,
							  IID_IDirectInput8,
							  (void**)&DirectInput,
							  NULL);
	HR(g_hr, "DirectInput8Create");

	// create the Direct Input devices
	g_hr = DirectInput->CreateDevice(GUID_SysKeyboard, &DIKeyboard, NULL);
	HR(g_hr, "DirectInput->CreateDevice");
	g_hr = DirectInput->CreateDevice(GUID_SysMouse, &DIMouse, NULL);
	HR(g_hr, "DirectInput->CreateDevice");

	// set the data format
	// tell the device what kind of input we are expecting
	g_hr = DIKeyboard->SetDataFormat(&c_dfDIKeyboard);
	g_hr = DIKeyboard->SetCooperativeLevel(g_hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

	g_hr = DIMouse->SetDataFormat(&c_dfDIMouse);
	g_hr = DIMouse->SetCooperativeLevel(g_hWnd, DISCL_EXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND);

	return true;
}

//--------------------------------------------------------------------------------------
// Detecting and Action accordingly
//--------------------------------------------------------------------------------------
void DetectInput(double time)
{
	DIMOUSESTATE mouseCurrState;
	BYTE keyboardState[256];

	DIKeyboard->Acquire();	// 取回键盘的控制权
	DIMouse->Acquire();		// 取回鼠标的控制权

	// get current state of device
	DIMouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&mouseCurrState);
	DIKeyboard->GetDeviceState(sizeof(keyboardState), (LPVOID)&keyboardState);

	if(keyboardState[DIK_ESCAPE] & 0x80)
	{
		PostMessage(g_hWnd, WM_DESTROY, 0, 0);
	}

	// Update g_Fov
	if(mouseCurrState.lZ != mouseLastState.lZ) // use mouse wheel to zoom in and out of the scene
	{
		g_Fov -= mouseCurrState.lZ * time * 0.1f;
	}

	// Transform Cubes
	if(keyboardState[DIK_LEFT] & 0x80)
	{
		g_rotz -= 1.0f * time;
	}
	if(keyboardState[DIK_RIGHT] & 0x80)
	{
		g_rotz += 1.0f * time;
	}
	if(keyboardState[DIK_UP] & 0x80)
	{
		g_rotx += 1.0f * time;
	}
	if(keyboardState[DIK_DOWN] & 0x80)
	{
		g_rotx -= 1.0f * time;
	}
	if(g_rotx > 6.28)
		g_rotx -= 6.28;
	else if(g_rotx < 0)
		g_rotx = 6.28 + g_rotx;
	if(g_rotz > 6.28)
		g_rotz -= 6.28;
	else if(g_rotz < 0)
		g_rotz = 6.28 + g_rotz;
	//if(mouseCurrState.lX != mouseLastState.lX)
	//{
	//	//scaleX -= (mouseCurrState.lX * 0.001f);
	//	scaleX -= mouseCurrState.lX  * time;
	//}
	//if(mouseCurrState.lY != mouseLastState.lY)
	//{
	//	//scaleY -= (mouseCurrState.lY * 0.001f);
	//	scaleY -= mouseCurrState.lY * time;
	//}

	///////////////**************new**************////////////////////
	// Move and Rotate Camera
	float speed = 15.0f * time;
	if(keyboardState[DIK_A] & 0x80)
	{
		g_CamMoveLeftRight -= speed;
	}
	if(keyboardState[DIK_D] & 0x80)
	{
		g_CamMoveLeftRight += speed;
	}
	if(keyboardState[DIK_W] & 0x80)
	{
		g_CamMoveBackForward += speed;
	}
	if(keyboardState[DIK_S] & 0x80)
	{
		g_CamMoveBackForward -= speed;
	}
	if((mouseCurrState.lX != mouseLastState.lX) || (mouseCurrState.lY != mouseLastState.lY))
	{
		g_CamPitch += mouseLastState.lY * 0.001f;
		g_CamYaw += mouseLastState.lX * 0.001f;
	}

	UpdateCamera();
	///////////////**************new**************////////////////////

	mouseLastState = mouseCurrState;
	return;
}

//--------------------------------------------------------------------------------------
// Update Camera, will be called every frame at the end of our DetectInput() function
//--------------------------------------------------------------------------------------
void UpdateCamera()
{
	g_CamRotationMatrix = XMMatrixRotationRollPitchYaw(g_CamPitch, g_CamYaw, 0.0f);
	g_CamTarget = XMVector3TransformCoord(g_DefaultForward, g_CamRotationMatrix);
	g_CamTarget = XMVector3Normalize(g_CamTarget);

	XMMATRIX RotateYTempMatrix = XMMatrixRotationY(g_CamYaw);

	// restrict camera to moving around on the X and Z plane.
	//g_CamRight = XMVector3TransformCoord(g_DefaultRight, RotateYTempMatrix);
	//g_CamUp = XMVector3TransformCoord(g_DefaultUp, RotateYTempMatrix);
	//g_CamForward = XMVector3TransformCoord(g_DefaultForward, RotateYTempMatrix);

	// camera can move freely
	g_CamRight = XMVector3Normalize(XMVector3TransformCoord(g_DefaultRight, g_CamRotationMatrix));
	g_CamUp = XMVector3Normalize(XMVector3TransformCoord(g_DefaultUp, g_CamRotationMatrix));
	g_CamForward = XMVector3Normalize(XMVector3TransformCoord(g_DefaultForward, g_CamRotationMatrix));

	g_CamPosition += g_CamMoveLeftRight * g_CamRight;
	g_CamPosition += g_CamMoveBackForward * g_CamForward;

	g_CamMoveLeftRight = 0.0f;
	g_CamMoveBackForward = 0.0f;

	g_CamTarget = g_CamPosition + g_CamTarget;
	g_View = XMMatrixLookAtLH(g_CamPosition, g_CamTarget, g_CamUp);
}

//--------------------------------------------------------------------------------------
// Update Scene
//--------------------------------------------------------------------------------------
void UpdateScene(double time)
{
	// Keep the cubes rotating
	//g_Rot += 0.0005f;
	g_Rot += 1.0f * time;
	if(g_Rot > 6.28f)
	{
		g_Rot = 0.0f;
	}

	// Reset g_WorldCube1
	g_WorldCube1 = XMMatrixIdentity();
	// Define cube1's world space matrix
	XMVECTOR rotXaxis = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR rotYaxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR rotZaxis = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	g_RotationY = XMMatrixRotationAxis(rotYaxis, g_Rot);
	g_RotationX = XMMatrixRotationAxis(rotXaxis, g_rotx);
	g_RotationZ = XMMatrixRotationAxis(rotZaxis, g_rotz);
	g_Translation = XMMatrixTranslation(4.0f, 0.0f, 0.0f);
	g_Scale = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	// Set cube1's world space use the transformations
	g_WorldCube1 = g_Scale * g_Translation * g_RotationY * g_RotationX * g_RotationZ; // Doing translation before rotation gives an orbit effect (公转)

	// Reset g_WorldCube2
	g_WorldCube2 = XMMatrixIdentity();
	// Define cube2's world space matrix
	g_Rotation = XMMatrixRotationAxis(rotYaxis, -g_Rot);
	g_Scale = XMMatrixScaling(1.0f, 1.5f, 1.0f);
	g_WorldCube2 = g_Scale * g_Rotation;

	// Reset g_GroundWorld
	g_GroundWorld = XMMatrixIdentity();
	// Define ground's world space matrix
	g_Scale = XMMatrixScaling(10.0f, 1.0f, 10.0f);
	g_Translation = XMMatrixTranslation(0.0f, -0.1f, 0.0f);
	g_GroundWorld = g_Scale * g_Translation;

	// Reset g_WorldSphere, for Skybox
	g_WorldSphere = XMMatrixIdentity();
	// Define sphere's world space matrix
	g_Scale = XMMatrixScaling(5.0f, 5.0f, 5.0f);
	g_Translation = XMMatrixTranslation(XMVectorGetX(g_CamPosition), XMVectorGetY(g_CamPosition), XMVectorGetZ(g_CamPosition));
	g_WorldSphere = g_Scale * g_Translation;

	// Reset g_WorldSphere2, for Sphere
	g_WorldSphere2 = XMMatrixIdentity();
	// Define sphere's world space matrix
	g_Scale = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	g_Translation = XMMatrixTranslation(0.0f, 2.0f, 0.0f);
	g_WorldSphere2 = g_Scale * g_Translation;

	///////////////**************new**************////////////////////
	// Update g_light's transform
	XMStoreFloat3(&g_light.pos, g_CamPosition);
	XMStoreFloat3(&g_light.dir, g_CamTarget - g_CamPosition);
	///////////////**************new**************////////////////////

	// Update projection
	g_Projection = XMMatrixPerspectiveFovLH(g_Fov * 3.14f, (float)g_Width / g_Height, 1.0f, 1000.0f);
}

//--------------------------------------------------------------------------------------
// Draw Scene
//--------------------------------------------------------------------------------------
void DrawScene()
{
	float bgColor[4] = { 0.0f, 0.1f, 0.1f, 1.0f };
	g_pd3d11DevCon->ClearRenderTargetView(g_pRenderTargetView, bgColor);
	g_pd3d11DevCon->ClearDepthStencilView(g_pDepthStencilView,
										  D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f,
										  0);  // Clear the depth/stencil view every frame!!!

	g_CBPerFrame.light = g_light;
	XMStoreFloat3(&g_CBPerFrame.camWorldPos, g_CamPosition);
	g_pd3d11DevCon->UpdateSubresource(g_pCBPerFrameBuffer, 0, 0, &g_CBPerFrame, 0, 0);
	g_pd3d11DevCon->PSSetConstantBuffers(0, 1, &g_pCBPerFrameBuffer);

	// Set our Render Target
	g_pd3d11DevCon->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// Set the default blend state (no blending) for opaque objects
	g_pd3d11DevCon->OMSetBlendState(0, 0, 0xffffffff); // 多重采样最多支持32个采样源, 参数 0xffffffff 表示不屏蔽任何采样员

	// Set Vertex and Pixel Shaders
	g_pd3d11DevCon->VSSetShader(g_pVS, 0, 0);
	g_pd3d11DevCon->PSSetShader(g_pPS, 0, 0);

	// Set the index buffer and vertex buffer
	g_pd3d11DevCon->IASetIndexBuffer(g_pCubeIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pd3d11DevCon->IASetVertexBuffers(0, 1, &g_pCubeVertBuffer, &stride, &offset);

#pragma region DrawFristCube
	g_WVP = g_WorldCube1 * g_View * g_Projection;
	g_CBPerObj.WVP = XMMatrixTranspose(g_WVP);
	g_CBPerObj.World = XMMatrixTranspose(g_WorldCube1);
	XMVECTOR det = XMMatrixDeterminant(g_WorldCube1);
	g_CBPerObj.WorldInvTranspose = XMMatrixInverse(&det, g_WorldCube1);
	g_pd3d11DevCon->UpdateSubresource(g_pCBPerObjectBuffer, 0, 0, &g_CBPerObj, 0, 0);
	g_pd3d11DevCon->VSSetConstantBuffers(0, 1, &g_pCBPerObjectBuffer);
	g_pd3d11DevCon->PSSetShaderResources(0, 1, &g_pCubeTexture);
	g_pd3d11DevCon->PSSetSamplers(0, 1, &g_pLinearClampSS);
	g_pd3d11DevCon->RSSetState(g_pCWcullMode);
	g_pd3d11DevCon->DrawIndexed(36, 0, 0);
#pragma endregion

#pragma region DrawSecondCube
	g_WVP = g_WorldCube2 * g_View * g_Projection;
	g_CBPerObj.WVP = XMMatrixTranspose(g_WVP);
	g_CBPerObj.World = XMMatrixTranspose(g_WorldCube2);
	det = XMMatrixDeterminant(g_WorldCube2);
	g_CBPerObj.WorldInvTranspose = XMMatrixInverse(&det, g_WorldCube2);
	g_pd3d11DevCon->UpdateSubresource(g_pCBPerObjectBuffer, 0, 0, &g_CBPerObj, 0, 0);
	g_pd3d11DevCon->VSSetConstantBuffers(0, 1, &g_pCBPerObjectBuffer);
	g_pd3d11DevCon->PSSetShaderResources(0, 1, &g_pCubeTexture);
	g_pd3d11DevCon->PSSetSamplers(0, 1, &g_pLinearClampSS);
	g_pd3d11DevCon->RSSetState(g_pCWcullMode);
	g_pd3d11DevCon->DrawIndexed(36, 0, 0);
#pragma endregion

#pragma region DrawGround
	// Set the index buffer and vertex buffer
	g_pd3d11DevCon->IASetIndexBuffer(g_pGroundIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	g_pd3d11DevCon->IASetVertexBuffers(0, 1, &g_pGroundVertBuffer, &stride, &offset);
	// Set the transforms
	g_WVP = g_GroundWorld * g_View * g_Projection;
	g_CBPerObj.WVP = XMMatrixTranspose(g_WVP);
	g_CBPerObj.World = XMMatrixTranspose(g_GroundWorld);
	det = XMMatrixDeterminant(g_GroundWorld);
	g_CBPerObj.WorldInvTranspose = XMMatrixInverse(&det, g_GroundWorld);
	g_pd3d11DevCon->UpdateSubresource(g_pCBPerObjectBuffer, 0, 0, &g_CBPerObj, 0, 0);
	g_pd3d11DevCon->VSSetConstantBuffers(0, 1, &g_pCBPerObjectBuffer);
	g_pd3d11DevCon->PSSetShaderResources(0, 1, &g_pGroundTexture);
	g_pd3d11DevCon->PSSetSamplers(0, 1, &g_pLinearWrapSS);
	// Set the RS states
	g_pd3d11DevCon->RSSetState(g_pCWcullMode);
	// Draw Ground
	g_pd3d11DevCon->DrawIndexed(6, 0, 0);
#pragma endrerion

	// Set the sphere index buffer and vertex buffer
	g_pd3d11DevCon->IASetIndexBuffer(g_pSphereIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	g_pd3d11DevCon->IASetVertexBuffers(0, 1, &g_pSphereVertexBuffer, &stride, &offset);
/*

#pragma region DrawSphere
	// Set the Vertex and Pixel Shaders
	g_pd3d11DevCon->VSSetShader(g_pReflectVS, 0, 0);
	g_pd3d11DevCon->PSSetShader(g_pReflectPS, 0, 0);
	// Set the transforms
	g_WVP = g_WorldSphere2 * g_View * g_Projection;
	g_CBPerObj.WVP = XMMatrixTranspose(g_WVP);
	g_CBPerObj.World = XMMatrixTranspose(g_WorldSphere2);
	det = XMMatrixDeterminant(g_WorldSphere2);
	g_CBPerObj.WorldInvTranspose = XMMatrixInverse(&det, g_WorldSphere2);
	g_pd3d11DevCon->UpdateSubresource(g_pCBPerObjectBuffer, 0, 0, &g_CBPerObj, 0, 0);
	g_pd3d11DevCon->VSSetConstantBuffers(0, 1, &g_pCBPerObjectBuffer);
	g_pd3d11DevCon->PSSetShaderResources(0, 1, &g_pSkyboxTexture);
	g_pd3d11DevCon->PSSetSamplers(0, 1, &g_pAnistropicWrapSS);
	// Set the RS states
	g_pd3d11DevCon->RSSetState(g_pNoCullMode);
	// Draw Sphere
	g_pd3d11DevCon->DrawIndexed(NumSphereFaces * 3, 0, 0);
#pragma endregion

#pragma region DrawSkybox
	// Set the Vertex and Pixel Shaders
	g_pd3d11DevCon->VSSetShader(g_pSkyboxVS, 0, 0);
	g_pd3d11DevCon->PSSetShader(g_pSkyboxPS, 0, 0);
	// Set the transforms
	g_WVP = g_WorldSphere * g_View * g_Projection;
	g_CBPerObj.WVP = XMMatrixTranspose(g_WVP);
	g_CBPerObj.World = XMMatrixTranspose(g_WorldSphere);
	g_pd3d11DevCon->UpdateSubresource(g_pCBPerObjectBuffer, 0, 0, &g_CBPerObj, 0, 0);
	g_pd3d11DevCon->VSSetConstantBuffers(0, 1, &g_pCBPerObjectBuffer);
	g_pd3d11DevCon->PSSetShaderResources(0, 1, &g_pSkyboxTexture);
	g_pd3d11DevCon->PSSetSamplers(0, 1, &g_pLinearClampSS);
	// Set the new depth/stencil and RS states
	g_pd3d11DevCon->RSSetState(g_pCWcullMode);
	g_pd3d11DevCon->OMSetDepthStencilState(DSLessEqual, 0);
	// Draw the Sky(天空盒最后渲染, 则那些被物体遮挡的部分不会被渲染出来, 避免重复渲染)
	g_pd3d11DevCon->DrawIndexed(NumSphereFaces * 3, 0, 0);
#pragma endregion

*/
	// Set the VS/PS shader and depth/stencil state to default state
	g_pd3d11DevCon->VSSetShader(g_pVS, 0, 0);
	g_pd3d11DevCon->PSSetShader(g_pPS, 0, 0);
	g_pd3d11DevCon->OMSetDepthStencilState(NULL, 0);
	///////////////**************new**************////////////////////

	RenderText(L" FPS: ", fps);

	// Present the backbuffer to the screen
	g_pSwapChain->Present(0, 0);
}

//--------------------------------------------------------------------------------------
// Message Processing Loop
//--------------------------------------------------------------------------------------
int messageloop()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	while(true)
	{
		BOOL PeekMessageL(
			LPMSG lpMsg,
			HWND hWnd,
			UINT wMsgFilterMin,
			UINT wMsgFilterMax,
			UINT wRemoveMsg
		);

		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// run game code

			if(!paused)
			{
				++frameCount;
				if(GetTime() > 1.0f)
				{
					fps = frameCount;
					frameCount = 0;
					StartTimer();
				}
				frameTime = GetFrameTime();
				DetectInput(frameTime);
				UpdateScene(frameTime);
			}
			else
			{
				// 暂停时, 将 frameTimeOld 置零
				// 通过修改GetFrameTime()程序,保证继续运行时,物体接着现在的状态运动,而不是突变
				frameTimeOld = 0;
				DetectInput(0.0);
				UpdateScene(0.0);
			}

			DrawScene();
		}
	}
	return msg.wParam;
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_KEYDOWN:
		if(wParam == VK_ESCAPE)
		{
			//Release the windows allocated memory
			DestroyWindow(hwnd);
		}
		else if(wParam == VK_PAUSE)
		{
			PauseTimer();
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

//--------------------------------------------------------------------------------------
// Release Objects
//--------------------------------------------------------------------------------------
void ReleaseObjects()
{
	g_pSwapChain->SetFullscreenState(false, NULL);
	PostMessage(g_hWnd, WM_DESTROY, 0, 0);
	// Release the COM Objects we created
	g_pSwapChain->Release();
	g_pd3d11Device->Release();
	g_pd3d11DevCon->Release();
	g_pRenderTargetView->Release();
	g_pCubeVertBuffer->Release();
	g_pCubeIndexBuffer->Release();
	g_pVS->Release();
	g_pPS->Release();
	g_pVS_Buffer->Release();
	g_pPS_Buffer->Release();
	g_pCubeTexture->Release();
	g_pGroundTexture->Release();
	///////////////**************new**************////////////////////
	g_pSphereVertexBuffer->Release();
	g_pSphereIndexBuffer->Release();
	g_pSkyboxVS->Release();
	g_pSkyboxPS->Release();
	g_pSkyboxVS_Buffer->Release();
	g_pSkyboxPS_Buffer->Release();
	g_pSkyboxTexture->Release();
	DSLessEqual->Release();
	g_pReflectVS->Release();
	g_pReflectPS->Release();
	g_pReflectVS_Buffer->Release();
	g_pReflectPS_Buffer->Release();
	g_pAnistropicWrapSS->Release();
	g_pLinearClampSS->Release();
	g_pLinearWrapSS->Release();
	///////////////**************new**************////////////////////
	g_pVertLayout->Release();
	g_pDepthStencilView->Release();
	g_pDepthStencilBuffer->Release();
	g_pCBPerObjectBuffer->Release();
	g_pCBPerFrameBuffer->Release();
	g_pD2D_PS->Release();
	g_pD2D_PS_Buffer->Release();
	g_pTransparency->Release();
	g_pCCWcullMode->Release();
	g_pCWcullMode->Release();
	g_pNoCullMode->Release();

	d3d101Device->Release();
	keyedMutex11->Release();
	keyedMutex10->Release();
	D2DRenderTarget->Release();
	Brush->Release();
	BackBuffer11->Release();
	sharedTex11->Release();
	DWriteFactory->Release();
	TextFormat->Release();
	d2dTexture->Release();

	DIKeyboard->Release();
	DIMouse->Release();
	DirectInput->Release();

	g_pGroundIndexBuffer->Release();
	g_pGroundVertBuffer->Release();
}

//--------------------------------------------------------------------------------------
// Start Timer
//--------------------------------------------------------------------------------------
void StartTimer()
{
	LARGE_INTEGER count;
	QueryPerformanceFrequency(&count);
	countsPerSecond = double(count.QuadPart);

	QueryPerformanceCounter(&count);
	CounterStart = count.QuadPart;
}

//--------------------------------------------------------------------------------------
// Get Time
//--------------------------------------------------------------------------------------
double GetTime()
{
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	return double(currentTime.QuadPart - CounterStart) / countsPerSecond;
}

//--------------------------------------------------------------------------------------
// Get Frame Time
//--------------------------------------------------------------------------------------
double GetFrameTime()
{
	LARGE_INTEGER currentTime;
	__int64 tickCount;
	QueryPerformanceCounter(&currentTime);

	// 若 frameTimeOld 置为 0, 则说明刚由暂停状态恢复
	if(frameTimeOld == 0)
		frameTimeOld = currentTime.QuadPart;

	tickCount = currentTime.QuadPart - frameTimeOld;
	frameTimeOld = currentTime.QuadPart;

	if(tickCount < 0.0f)
		tickCount = 0.0f;

	return double(tickCount) / countsPerSecond;
}

//--------------------------------------------------------------------------------------
// Pause Timer 
//--------------------------------------------------------------------------------------
void PauseTimer()
{
	if(!paused)
		paused = true;
	else
		paused = false;
}

//--------------------------------------------------------------------------------------
// Initialize Direct3D 10.1, Direct2D, and DirectWrite Using a Shared Surface
//--------------------------------------------------------------------------------------
bool InitD2D_D3D101_DWrite(IDXGIAdapter1 *Adapter)
{
	//Create our Direc3D 10.1 Device///////////////////////////////////////////////////////////////////////////////////////
	g_hr = D3D10CreateDevice1(Adapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, D3D10_CREATE_DEVICE_DEBUG | D3D10_CREATE_DEVICE_BGRA_SUPPORT,
							  D3D10_FEATURE_LEVEL_9_3, D3D10_1_SDK_VERSION, &d3d101Device);
	HR(g_hr, "D3D10CreateDevice1");

	//Create Shared Texture that Direct3D 10.1 will render on//////////////////////////////////////////////////////////////
	D3D11_TEXTURE2D_DESC sharedTexDesc;

	ZeroMemory(&sharedTexDesc, sizeof(sharedTexDesc));

	sharedTexDesc.Width = g_Width;
	sharedTexDesc.Height = g_Height;
	sharedTexDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sharedTexDesc.MipLevels = 1;
	sharedTexDesc.ArraySize = 1;
	sharedTexDesc.SampleDesc.Count = 1;
	sharedTexDesc.Usage = D3D11_USAGE_DEFAULT;
	sharedTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	sharedTexDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

	g_hr = g_pd3d11Device->CreateTexture2D(&sharedTexDesc, NULL, &sharedTex11);
	HR(g_hr, "g_pd3d11Device->CreateTexture2D");

	// Get the keyed mutex for the shared texture (for D3D11)///////////////////////////////////////////////////////////////
	g_hr = sharedTex11->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&keyedMutex11);
	HR(g_hr, "sharedTex11->QueryInterface");

	// Get the shared handle needed to open the shared texture in D3D10.1///////////////////////////////////////////////////
	IDXGIResource *sharedResource10;
	HANDLE sharedHandle10;

	g_hr = sharedTex11->QueryInterface(__uuidof(IDXGIResource), (void**)&sharedResource10);
	HR(g_hr, "sharedTex11->QueryInterface");

	g_hr = sharedResource10->GetSharedHandle(&sharedHandle10);
	HR(g_hr, "sharedResource10->GetSharedHandle");

	sharedResource10->Release();

	// Open the surface for the shared texture in D3D10.1///////////////////////////////////////////////////////////////////
	IDXGISurface1 *sharedSurface10;

	g_hr = d3d101Device->OpenSharedResource(sharedHandle10, __uuidof(IDXGISurface1), (void**)(&sharedSurface10));
	HR(g_hr, "d3d101Device->OpenSharedResource");

	g_hr = sharedSurface10->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&keyedMutex10);
	HR(g_hr, "sharedSurface10->QueryInterface");

	// Create D2D factory///////////////////////////////////////////////////////////////////////////////////////////////////
	ID2D1Factory *D2DFactory;
	g_hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), (void**)&D2DFactory);
	HR(g_hr, "D2D1CreateFactory");

	D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties;

	ZeroMemory(&renderTargetProperties, sizeof(renderTargetProperties));

	renderTargetProperties.type = D2D1_RENDER_TARGET_TYPE_HARDWARE;
	renderTargetProperties.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED);

	g_hr = D2DFactory->CreateDxgiSurfaceRenderTarget(sharedSurface10, &renderTargetProperties, &D2DRenderTarget);
	HR(g_hr, "D2DFactory->CreateDxgiSurfaceRenderTarget");

	sharedSurface10->Release();
	D2DFactory->Release();

	// Create a solid color brush to draw something with		
	g_hr = D2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 0.0f, 1.0f), &Brush);
	HR(g_hr, "D2DRenderTarget->CreateSolidColorBrush");

	//DirectWrite///////////////////////////////////////////////////////////////////////////////////////////////////////////
	g_hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
							   reinterpret_cast<IUnknown**>(&DWriteFactory));
	HR(g_hr, "DWriteCreateFactory");

	g_hr = DWriteFactory->CreateTextFormat(
		L"Script",
		NULL,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		24.0f,
		L"en-us",
		&TextFormat
	);
	HR(g_hr, "DWriteFactory->CreateTextFormat");

	g_hr = TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	HR(g_hr, "TextFormat->SetTextAlignment");
	g_hr = TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	HR(g_hr, "TextFormat->SetParagraphAlignment");

	d3d101Device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST); // keep the D3D 10.1 Debug Output Quiet ;)
	return true;
}

//--------------------------------------------------------------------------------------
// Initializing the Scene to Display D2D
//--------------------------------------------------------------------------------------
bool InitD2DScreenTexture()
{
	//Create the vertex buffer
	Vertex v[] =
	{
		// Front Face
		Vertex(-1.0f, -1.0f, -1.0f, 0.0f, 1.0f,-1.0f, -1.0f, -1.0f),
		Vertex(-1.0f,  1.0f, -1.0f, 0.0f, 0.0f,-1.0f,  1.0f, -1.0f),
		Vertex(1.0f,  1.0f, -1.0f, 1.0f, 0.0f, 1.0f,  1.0f, -1.0f),
		Vertex(1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f),
	};

	DWORD indices[] = {
		// Front Face
		0,  1,  2,
		0,  2,  3,
	};

	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(DWORD) * 2 * 3;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;

	iinitData.pSysMem = indices;
	g_pd3d11Device->CreateBuffer(&indexBufferDesc, &iinitData, &d2dIndexBuffer);


	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * 4;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;

	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
	vertexBufferData.pSysMem = v;
	g_hr = g_pd3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &d2dVertBuffer);
	HR(g_hr, "g_pd3d11Device->CreateBuffer");

	//Create A shader resource view from the texture D2D will render to,
	//So we can use it to texture a square which overlays our scene
	g_pd3d11Device->CreateShaderResourceView(sharedTex11, NULL, &d2dTexture);

	return true;
}

//--------------------------------------------------------------------------------------
// Render the Font
//--------------------------------------------------------------------------------------
void RenderText(std::wstring text, int inInt)
{
	//Release the D3D 11 Device
	keyedMutex11->ReleaseSync(0);

	//Use D3D10.1 device
	keyedMutex10->AcquireSync(0, 5);

	//Draw D2D content		
	D2DRenderTarget->BeginDraw();

	//Clear D2D Background
	D2DRenderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

	//Create our string
	std::wostringstream printString;
	printString << text << inInt;
	printText = printString.str();

	//Set the Font Color
	D2D1_COLOR_F FontColor = D2D1::ColorF(0.0f, 1.0f, 0.0f, 1.0f);

	//Set the brush color D2D will use to draw with
	Brush->SetColor(FontColor);

	//Create the D2D Render Area
	D2D1_RECT_F layoutRect = D2D1::RectF(0, 0, g_Width, g_Height);

	//Draw the Text
	D2DRenderTarget->DrawText(
		printText.c_str(),
		wcslen(printText.c_str()),
		TextFormat,
		layoutRect,
		Brush
	);

	D2DRenderTarget->EndDraw();

	//Release the D3D10.1 Device
	keyedMutex10->ReleaseSync(1);

	//Use the D3D11 Device
	keyedMutex11->AcquireSync(1, 5);

	//Use the shader resource representing the direct2d render target
	//to texture a square which is rendered in screen space so it
	//overlays on top of our entire scene. We use alpha blending so
	//that the entire background of the D2D render target is "invisible",
	//And only the stuff we draw with D2D will be visible (the text)

	//Set the blend state for D2D render target texture objects
	g_pd3d11DevCon->OMSetBlendState(g_pTransparency, NULL, 0xffffffff);

	//Set d2d's pixel shader so lighting calculations are not done
	g_pd3d11DevCon->VSSetShader(g_pVS, 0, 0);
	g_pd3d11DevCon->PSSetShader(g_pD2D_PS, 0, 0);

	//Set the d2d Index buffer
	g_pd3d11DevCon->IASetIndexBuffer(d2dIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	//Set the d2d vertex buffer
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pd3d11DevCon->IASetVertexBuffers(0, 1, &d2dVertBuffer, &stride, &offset);

	g_WVP = XMMatrixIdentity();
	g_CBPerObj.WVP = XMMatrixTranspose(g_WVP);
	///////////////**************new**************////////////////////
	g_CBPerObj.World = XMMatrixTranspose(g_WVP); // set World Matrix to Identity
	///////////////**************new**************////////////////////
	g_pd3d11DevCon->UpdateSubresource(g_pCBPerObjectBuffer, 0, NULL, &g_CBPerObj, 0, 0);
	g_pd3d11DevCon->VSSetConstantBuffers(0, 1, &g_pCBPerObjectBuffer);
	g_pd3d11DevCon->PSSetShaderResources(0, 1, &d2dTexture);
	g_pd3d11DevCon->PSSetSamplers(0, 1, &g_pLinearClampSS);

	g_pd3d11DevCon->RSSetState(g_pCWcullMode);

	//Draw the second cube
	g_pd3d11DevCon->DrawIndexed(6, 0, 0);
}

// LatLines: 纬线个数
// LongLines: 经线个数
void CreateSphere(int LatLines, int LongLines)
{
	NumSphereVertices = ((LatLines - 2) * LongLines) + 2;
	NumSphereFaces = ((LatLines - 3)*(LongLines) * 2) + (LongLines * 2);

	float sphereYaw = 0.0f;
	float spherePitch = 0.0f;

	std::vector<Vertex> vertices(NumSphereVertices);

	XMVECTOR currVertPos = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	vertices[0].pos.x = 0.0f;
	vertices[0].pos.y = 0.0f;
	vertices[0].pos.z = 1.0f;

	for(DWORD i = 0; i < LatLines - 2; ++i)
	{
		spherePitch = (i + 1) * (3.14 / (LatLines - 1));
		g_RotationX = XMMatrixRotationX(spherePitch);
		for(DWORD j = 0; j < LongLines; ++j)
		{
			sphereYaw = j * (6.28 / (LongLines));
			g_RotationY = XMMatrixRotationZ(sphereYaw);
			currVertPos = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), (g_RotationX * g_RotationY));
			currVertPos = XMVector3Normalize(currVertPos);
			vertices[i*LongLines + j + 1].pos.x = XMVectorGetX(currVertPos);
			vertices[i*LongLines + j + 1].pos.y = XMVectorGetY(currVertPos);
			vertices[i*LongLines + j + 1].pos.z = XMVectorGetZ(currVertPos);
		}
	}

	vertices[NumSphereVertices - 1].pos.x = 0.0f;
	vertices[NumSphereVertices - 1].pos.y = 0.0f;
	vertices[NumSphereVertices - 1].pos.z = -1.0f;

	// 局部空间下, 球上每一个顶点的法线方向和顶点坐标相同.
	for(DWORD i = 0; i < vertices.size(); ++i)
	{
		vertices[i].normal = vertices[i].pos;
	}

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * NumSphereVertices;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;

	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
	vertexBufferData.pSysMem = &vertices[0];
	g_hr = g_pd3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &g_pSphereVertexBuffer);


	std::vector<DWORD> indices(NumSphereFaces * 3);

	int k = 0;
	// 这样构造会有一个孔
	//for(DWORD l = 0; l < LongLines - 1; ++l)
	//{
	//	indices[k] = 0;
	//	indices[k + 1] = l + 1;
	//	indices[k + 2] = l + 2;
	//	k += 3;
	//}
	//indices[k] = 0;
	//indices[k + 1] = LongLines;
	//indices[k + 2] = 1;
	//k += 3;

	for(DWORD l = 0; l < LongLines - 1; ++l)
	{
		indices[k] = 0;
		indices[k + 2] = l + 1;
		indices[k + 1] = l + 2;
		k += 3;
	}

	indices[k] = 0;
	indices[k + 2] = LongLines;
	indices[k + 1] = 1;
	k += 3;

	for(DWORD i = 0; i < LatLines - 3; ++i)
	{
		for(DWORD j = 0; j < LongLines - 1; ++j)
		{
			indices[k] = i * LongLines + j + 1;
			indices[k + 1] = i * LongLines + j + 2;
			indices[k + 2] = (i + 1)*LongLines + j + 1;

			indices[k + 3] = (i + 1)*LongLines + j + 1;
			indices[k + 4] = i * LongLines + j + 2;
			indices[k + 5] = (i + 1)*LongLines + j + 2;

			k += 6; // next quad
		}

		indices[k] = (i*LongLines) + LongLines;
		indices[k + 1] = (i*LongLines) + 1;
		indices[k + 2] = ((i + 1)*LongLines) + LongLines;

		indices[k + 3] = ((i + 1)*LongLines) + LongLines;
		indices[k + 4] = (i*LongLines) + 1;
		indices[k + 5] = ((i + 1)*LongLines) + 1;

		k += 6;
	}

	// 这样构造会有一个孔
	//for(DWORD l = 0; l < LongLines - 1; ++l)
	//{
	//	indices[k] = NumSphereVertices - 1;
	//	indices[k + 1] = (NumSphereVertices - 1) - (l + 1);
	//	indices[k + 2] = (NumSphereVertices - 1) - (l + 2);
	//	k += 3;
	//}
	//indices[k] = NumSphereVertices - 1;
	//indices[k + 1] = (NumSphereVertices - 1) - LongLines;
	//indices[k + 2] = NumSphereVertices - 2;

	for(DWORD l = 0; l < LongLines - 1; ++l)
	{
		indices[k] = NumSphereVertices - 1;
		indices[k + 2] = (NumSphereVertices - 1) - (l + 1);
		indices[k + 1] = (NumSphereVertices - 1) - (l + 2);
		k += 3;
	}

	indices[k] = NumSphereVertices - 1;
	indices[k + 2] = (NumSphereVertices - 1) - LongLines;
	indices[k + 1] = NumSphereVertices - 2;

	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(DWORD) * NumSphereFaces * 3;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;

	iinitData.pSysMem = &indices[0];
	g_pd3d11Device->CreateBuffer(&indexBufferDesc, &iinitData, &g_pSphereIndexBuffer);
}