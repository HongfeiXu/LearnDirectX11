//--------------------------------------------------------------------------------------
// Include and link appropriate libraries and headers
//--------------------------------------------------------------------------------------
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "d3dx10.lib")
#pragma comment(lib, "DXErr.lib") // the dxerr.lib is no longer compatible with Visual Studio 2015 and later Visual Studio..
#pragma comment(lib, "legacy_stdio_definitions.lib") // so we link this, https://stackoverflow.com/questions/31053670/unresolved-external-symbol-vsnprintf-in-dxerr-lib

#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include <DxErr.h>
#include <cassert>


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct CBPerObject
{
	XMMATRIX WVP;
};

struct Vertex {
	Vertex() {}
	Vertex(float x, float y, float z,
		   float u, float v)
		: pos(x, y, z), texCoord(u, v)
	{
	}

	XMFLOAT3 pos;
	XMFLOAT2 texCoord;
};


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
IDXGISwapChain*									g_pSwapChain;
ID3D11Device*									g_pd3d11Device;					// 用于检测显示适配器功能和分配资源
ID3D11DeviceContext*							g_pd3d11DevCon;			// 用于设置管线状态, 将资源绑定到图形管线和生成渲染命令
ID3D11RenderTargetView*							g_pRenderTargetView;	// 渲染目标视图(资源不能直接绑定到一个管线阶段, 必须为资源创建资源视图, 然后绑定到不同的管线阶段)
ID3D11DepthStencilView*							g_pDepthStencilView;
ID3D11Texture2D*								g_pDepthStencilBuffer;
ID3D11Buffer*									g_pSquareVertBuffer;
ID3D11Buffer*									g_pSquareIndexBuffer;
ID3D11VertexShader*								g_pVS;
ID3D11PixelShader*								g_pPS;
ID3D10Blob*										g_pVS_Buffer; // hold the information about vertex shader, use this buffer to create the actual shader
ID3D10Blob*										g_pPS_Buffer; // hold the information about pixel shader, use this buffer to create the actual shader
ID3D11InputLayout*								g_pVertLayout;
ID3D11Buffer*									g_pCBPerObjectBuffer; // store our constant buffer variables to send to the actual constant buffer in the effect file
CBPerObject										g_CBPerObj;
ID3D11ShaderResourceView*						g_pCubeTexture;		// 当把一个纹理作为一个着色器资源时, 需要创建着色器视图
ID3D11SamplerState*								g_pCubesTexSamplerState;
ID3D11BlendState*								g_pTransparency;
ID3D11RasterizerState*							g_pCCWcullMode;	// counter clockwise culling
ID3D11RasterizerState*							g_pCWcullMode;	// clockwise culling


LPCTSTR						g_WndClassName = L"firstwindow";
HWND						g_hWnd = NULL;
HRESULT						g_hr;
const int					g_Width = 800;
const int					g_Height = 800;
bool						g_Enable4xMsaa = true;

XMMATRIX	 				g_WVP;
XMMATRIX	 				g_World1;
XMMATRIX	 				g_World2;
XMMATRIX	 				g_View;
XMMATRIX	 				g_Projection;
XMVECTOR	 				g_CamPosition;
XMVECTOR	 				g_CamTarget;
XMVECTOR	 				g_CamUp;
XMMATRIX	 				g_Rotation;
XMMATRIX	 				g_Scale;
XMMATRIX	 				g_Translation;
float	 					g_Rot = 0.01f; // keep track of rotation

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
bool InitializeDirect3d11App(HINSTANCE hInstance);
void ReleaseObjects();
bool InitScene();
void UpdateScene();
void DrawScene();
bool InitializeWindow(HINSTANCE hInstance, int ShowWnd, int width, int height, bool windowed);
int messageloop();
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
		L"SimpleFont",
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
// Initialize Direct3D
//--------------------------------------------------------------------------------------
bool InitializeDirect3d11App(HINSTANCE hInstance)
{
	// Create Device and Context (+ DX11 龙书中译 P63)
	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel;
	g_hr = D3D11CreateDevice(
		0,
		D3D_DRIVER_TYPE_HARDWARE,
		0,
		createDeviceFlags,
		0, 0,
		D3D11_SDK_VERSION,
		&g_pd3d11Device,
		&featureLevel,
		&g_pd3d11DevCon);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("D3D11CreateDevice"), MB_OK);
		return 0;
	}
	if(featureLevel != D3D_FEATURE_LEVEL_11_0)
	{
		MessageBox(NULL, L"Direct3D FeatureLevel 11 unsupported.", 0, 0);
		return 0;
	}

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
	bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;

	// Create our SwapChain (<-> DX11 龙书中译 P68)
	IDXGIDevice * dxgiDevice = 0;
	g_hr = g_pd3d11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("d3d11Device->QueryInterface"), MB_OK);
		return 0;
	}
	IDXGIAdapter* dxgiAdapter = 0;
	g_hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("dxgiDevice->GetParent"), MB_OK);
		return 0;
	}
	IDXGIFactory* dxgiFactory = 0;
	g_hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("dxgiAdapter->GetParent"), MB_OK);
		return 0;
	}
	dxgiFactory->CreateSwapChain(g_pd3d11Device, &swapChainDesc, &g_pSwapChain);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("dxgiFactory->CreateSwapChain"), MB_OK);
		return 0;
	}
	dxgiDevice->Release();
	dxgiAdapter->Release();
	dxgiFactory->Release();

	// Create Render Target View
	ID3D11Texture2D* backBuffer;
	g_hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)); // 获取一个交换链的后台缓冲区指针
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("SwapChain->GetBuffer"), MB_OK);
		return 0;
	}
	g_hr = g_pd3d11Device->CreateRenderTargetView(backBuffer, 0, &g_pRenderTargetView); // 创建渲染目标视图
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("d3d11Device->CreateRenderTargetView"), MB_OK);
		return 0;
	}
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
	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	UINT numElements = ARRAYSIZE(layout); // hold the size of our input layout array

	// Compile Shader from shader file
	g_hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "VS", "vs_5_0", 0, 0, 0, &g_pVS_Buffer, 0, 0);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("D3DX11CompileFromFile"), MB_OK);
		return 0;
	}

	g_hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "PS", "ps_5_0", 0, 0, 0, &g_pPS_Buffer, 0, 0);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("D3DX11CompileFromFile"), MB_OK);
		return 0;
	}

	// Create the Shader Objects
	g_hr = g_pd3d11Device->CreateVertexShader(g_pVS_Buffer->GetBufferPointer(), g_pVS_Buffer->GetBufferSize(), NULL, &g_pVS);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("d3d11Device->CreateVertexShader"), MB_OK);
		return 0;
	}
	g_hr = g_pd3d11Device->CreatePixelShader(g_pPS_Buffer->GetBufferPointer(), g_pPS_Buffer->GetBufferSize(), NULL, &g_pPS);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("d3d11Device->CreatePixelShader"), MB_OK);
		return 0;
	}

	// Set Vertex and Pixel Shaders
	g_pd3d11DevCon->VSSetShader(g_pVS, 0, 0);
	g_pd3d11DevCon->PSSetShader(g_pPS, 0, 0);

	Vertex vertices[] =
	{
		// Front Face
		Vertex(-1.0f, -1.0f, -1.0f, 0.0f, 1.0f),
		Vertex(-1.0f,  1.0f, -1.0f, 0.0f, 0.0f),
		Vertex(1.0f,  1.0f, -1.0f, 1.0f, 0.0f),
		Vertex(1.0f, -1.0f, -1.0f, 1.0f, 1.0f),

		// Back Face
		Vertex(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f),
		Vertex(1.0f, -1.0f, 1.0f, 0.0f, 1.0f),
		Vertex(1.0f,  1.0f, 1.0f, 0.0f, 0.0f),
		Vertex(-1.0f,  1.0f, 1.0f, 1.0f, 0.0f),

		// Top Face
		Vertex(-1.0f, 1.0f, -1.0f, 0.0f, 1.0f),
		Vertex(-1.0f, 1.0f,  1.0f, 0.0f, 0.0f),
		Vertex(1.0f, 1.0f,  1.0f, 1.0f, 0.0f),
		Vertex(1.0f, 1.0f, -1.0f, 1.0f, 1.0f),

		// Bottom Face
		Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f),
		Vertex(1.0f, -1.0f, -1.0f, 0.0f, 1.0f),
		Vertex(1.0f, -1.0f,  1.0f, 0.0f, 0.0f),
		Vertex(-1.0f, -1.0f,  1.0f, 1.0f, 0.0f),

		// Left Face
		Vertex(-1.0f, -1.0f,  1.0f, 0.0f, 1.0f),
		Vertex(-1.0f,  1.0f,  1.0f, 0.0f, 0.0f),
		Vertex(-1.0f,  1.0f, -1.0f, 1.0f, 0.0f),
		Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f),

		// Right Face
		Vertex(1.0f, -1.0f, -1.0f, 0.0f, 1.0f),
		Vertex(1.0f,  1.0f, -1.0f, 0.0f, 0.0f),
		Vertex(1.0f,  1.0f,  1.0f, 1.0f, 0.0f),
		Vertex(1.0f, -1.0f,  1.0f, 1.0f, 1.0f),
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

	// Create the index buffer
	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

	indexBufferDesc.ByteWidth = sizeof(indices);
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER; // this is a index buffer
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexBufferData;
	ZeroMemory(&indexBufferData, sizeof(indexBufferData));

	indexBufferData.pSysMem = indices;
	g_hr = g_pd3d11Device->CreateBuffer(&indexBufferDesc, &indexBufferData, &g_pSquareIndexBuffer);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("d3d11Device->CreateBuffer"), MB_OK);
		return 0;
	}

	// Set the index buffer
	g_pd3d11DevCon->IASetIndexBuffer(g_pSquareIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Create the vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

	vertexBufferDesc.ByteWidth = sizeof(vertices);
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; // this is a vertex buffer
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;
	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));

	vertexBufferData.pSysMem = vertices;
	g_hr = g_pd3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &g_pSquareVertBuffer);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("d3d11Device->CreateBuffer"), MB_OK);
		return 0;
	}

	// Set the vertex buffer
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pd3d11DevCon->IASetVertexBuffers(0, 1, &g_pSquareVertBuffer, &stride, &offset);

	// Create the Input Layout
	g_hr = g_pd3d11Device->CreateInputLayout(layout, numElements, g_pVS_Buffer->GetBufferPointer(),
										g_pVS_Buffer->GetBufferSize(), &g_pVertLayout);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("d3d11Device->CreateInputLayout"), MB_OK);
		return 0;
	}

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

	// Create the buffer to send to the cbuffer in effect file
	D3D11_BUFFER_DESC cbbd;
	ZeroMemory(&cbbd, sizeof(cbbd));

	cbbd.ByteWidth = sizeof(CBPerObject);
	cbbd.Usage = D3D11_USAGE_DEFAULT;
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;
	cbbd.StructureByteStride = 0;

	g_hr = g_pd3d11Device->CreateBuffer(&cbbd, 0, &g_pCBPerObjectBuffer);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("d3d11Device->CreateBuffer"), MB_OK);
		return 0;
	}

	// Camera infomation(a simple static camera)
	g_CamPosition = XMVectorSet(0.0f, 3.0f, -8.0f, 0.0f);
	g_CamTarget = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	g_CamUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	// Set the View matrix
	g_View = XMMatrixLookAtLH(g_CamPosition, g_CamTarget, g_CamUp);

	// Set the Project matrix
	g_Projection = XMMatrixPerspectiveFovLH(0.4f * 3.14f, (float)g_Width / g_Height, 1.0f, 1000.0f);

	// Load the Texture from a file
	g_hr = D3DX11CreateShaderResourceViewFromFile(g_pd3d11Device, L"braynzar.jpg", NULL, NULL, &g_pCubeTexture, NULL);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("D3DX11CreateShaderResourceViewFromFile"), MB_OK);
		return 0;
	}
	
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
	g_hr = g_pd3d11Device->CreateSamplerState(&sampDesc, &g_pCubesTexSamplerState);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("d3d11Device->CreateSamplerState"), MB_OK);
		return 0;
	}

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

	blendDesc.AlphaToCoverageEnable = false;	// 一种多重采样技术, 在渲染植物叶子或者铁丝网纹理时非常有用
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget[0] = rtbd;

	g_pd3d11Device->CreateBlendState(&blendDesc, &g_pTransparency);

	// Describe the (Rasterizer) Render State
	D3D11_RASTERIZER_DESC cmdesc;
	ZeroMemory(&cmdesc, sizeof(D3D11_RASTERIZER_DESC));

	cmdesc.FillMode = D3D11_FILL_SOLID;
	cmdesc.CullMode = D3D11_CULL_BACK;
	cmdesc.MultisampleEnable = TRUE; // 开启多重采样抗锯齿, 当 enable4xMsaa 也为 true 时有效.
	cmdesc.FrontCounterClockwise = true;
	g_hr = g_pd3d11Device->CreateRasterizerState(&cmdesc, &g_pCCWcullMode);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("d3d11Device->CreateRasterizerState"), MB_OK);
		return 0;
	}

	cmdesc.FrontCounterClockwise = false;
	g_hr = g_pd3d11Device->CreateRasterizerState(&cmdesc, &g_pCWcullMode);
	if(FAILED(g_hr))
	{
		MessageBox(NULL, DXGetErrorDescription(g_hr),
				   TEXT("d3d11Device->CreateRasterizerState"), MB_OK);
		return 0;
	}

	return true;
}

//--------------------------------------------------------------------------------------
// Update Scene
//--------------------------------------------------------------------------------------
void UpdateScene()
{
	// Keep the cubes rotating
	g_Rot += 0.0005f;
	if(g_Rot > 6.28f)
	{
		g_Rot = 0.0f;
	}

	// Reset cube1World
	g_World1 = XMMatrixIdentity();

	// Define cube1's world space matrix
	XMVECTOR rotaxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	g_Rotation = XMMatrixRotationAxis(rotaxis, g_Rot);
	g_Translation = XMMatrixTranslation(4.0f, 0.0f, 0.0f);
	g_Scale = XMMatrixScaling(0.5f, 0.5f, 0.5f);

	// Set cube1's world space use the transformations
	g_World1 = g_Scale * g_Translation * g_Rotation; // Doing translation before rotation gives an orbit effect (公转)
	//cube1World = Scale * Rotation * Translation; // Doing rotation before translation gives an spinning effect (自转)

	 // Reset cube2World
	g_World2 = XMMatrixIdentity();

	// Define cube2's world space matrix
	rotaxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	g_Rotation = XMMatrixRotationAxis(rotaxis, -g_Rot);
	g_Scale = XMMatrixScaling(1.3f, 1.3f, 1.3f);

	// Set cube2's world space matrix
	g_World2 = g_Scale * g_Rotation;
}

//--------------------------------------------------------------------------------------
// Draw Scene
//--------------------------------------------------------------------------------------
void DrawScene()
{
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	g_pd3d11DevCon->ClearRenderTargetView(g_pRenderTargetView, bgColor);
	g_pd3d11DevCon->ClearDepthStencilView(g_pDepthStencilView,
									   D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f,
									   0);  // Clear the depth/stencil view every frame!!!

	// "fine-tune" the blending equation
	float blendFactor[] = { 0.75f, 0.75f, 0.75f, 1.0f };

	// Set the default blend state (no blending) for opaque objects
	g_pd3d11DevCon->OMSetBlendState(0, 0, 0xffffffff); // 多重采样最多支持32个采样源, 参数 0xffffffff 表示不屏蔽任何采样员

	// Render opaque objects here //

	// Set the blend state for transparent objects
	g_pd3d11DevCon->OMSetBlendState(g_pTransparency, blendFactor, 0xffffffff);

	// Transparency Depth Ordering
	XMVECTOR cubePos = XMVectorZero();
	cubePos = XMVector3TransformCoord(cubePos, g_World1);
	auto temp = cubePos - g_CamPosition;
	float cube1Dist = XMVectorGetX(XMVector3LengthSq(temp));

	cubePos = XMVectorZero();
	cubePos = XMVector3TransformCoord(cubePos, g_World2);
	temp = cubePos - g_CamPosition;
	float cube2Dist = XMVectorGetX(XMVector3LengthSq(temp));

	if(cube1Dist < cube2Dist)	// the further one should be rendered first
	{
		auto tempMatrix = g_World1;
		g_World1 = g_World2;
		g_World2 = tempMatrix;
	}

	// Set the World/View/Projection matrix, then send it to constant buffer in effect file
	g_WVP = g_World1 * g_View * g_Projection;
	g_CBPerObj.WVP = XMMatrixTranspose(g_WVP);
	g_pd3d11DevCon->UpdateSubresource(g_pCBPerObjectBuffer, 0, 0, &g_CBPerObj, 0, 0);
	g_pd3d11DevCon->VSSetConstantBuffers(0, 1, &g_pCBPerObjectBuffer);
	g_pd3d11DevCon->PSSetShaderResources(0, 1, &g_pCubeTexture);
	g_pd3d11DevCon->PSSetSamplers(0, 1, &g_pCubesTexSamplerState);
	
	// Draw the first cube, render back face before front face
	g_pd3d11DevCon->RSSetState(g_pCCWcullMode);
	g_pd3d11DevCon->DrawIndexed(36, 0, 0);
	g_pd3d11DevCon->RSSetState(g_pCWcullMode);
	g_pd3d11DevCon->DrawIndexed(36, 0, 0);

	// Set the World/View/Projection matrix, then send it to constant buffer in effect file
	g_WVP = g_World2 * g_View * g_Projection;
	g_CBPerObj.WVP = XMMatrixTranspose(g_WVP);
	g_pd3d11DevCon->UpdateSubresource(g_pCBPerObjectBuffer, 0, 0, &g_CBPerObj, 0, 0);
	g_pd3d11DevCon->VSSetConstantBuffers(0, 1, &g_pCBPerObjectBuffer);
	g_pd3d11DevCon->PSSetShaderResources(0, 1, &g_pCubeTexture);
	g_pd3d11DevCon->PSSetSamplers(0, 1, &g_pCubesTexSamplerState);

	// Draw the second cube
	g_pd3d11DevCon->RSSetState(g_pCCWcullMode);
	g_pd3d11DevCon->DrawIndexed(36, 0, 0);
	g_pd3d11DevCon->RSSetState(g_pCWcullMode);
	g_pd3d11DevCon->DrawIndexed(36, 0, 0);

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
			UpdateScene();
			DrawScene();
		}
	}
	return msg.wParam;
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd,
						 UINT msg,
						 WPARAM wParam,
						 LPARAM lParam)
{
	switch(msg)
	{
	case WM_KEYDOWN:
		if(wParam == VK_ESCAPE)
		{
			if(MessageBox(0, L"Are you sure you want to exit?",
						  L"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES)

				//Release the windows allocated memory
				DestroyWindow(hwnd);
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd,
						 msg,
						 wParam,
						 lParam);
}

//--------------------------------------------------------------------------------------
// Release Objects
//--------------------------------------------------------------------------------------
void ReleaseObjects()
{
	// Release the COM Objects we created
	g_pSwapChain->Release();
	g_pd3d11Device->Release();
	g_pd3d11DevCon->Release();
	g_pRenderTargetView->Release();
	g_pSquareVertBuffer->Release();
	g_pSquareIndexBuffer->Release();
	g_pVS->Release();
	g_pPS->Release();
	g_pVS_Buffer->Release();
	g_pPS_Buffer->Release();
	g_pVertLayout->Release();
	g_pDepthStencilView->Release();
	g_pDepthStencilBuffer->Release();
	g_pCBPerObjectBuffer->Release();
	g_pTransparency->Release();
	g_pCCWcullMode->Release();
	g_pCWcullMode->Release();
}
