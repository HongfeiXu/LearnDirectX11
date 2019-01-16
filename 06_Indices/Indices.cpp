//Include and link appropriate libraries and headers//
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "d3dx10.lib")
#pragma comment(lib, "DXErr.lib")

#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#include <xnamath.h>
#include <DxErr.h>
#include <cassert>

//Global Declarations//
IDXGISwapChain* SwapChain;
ID3D11Device* d3d11Device;					// 用于检测显示适配器功能和分配资源
ID3D11DeviceContext* d3d11DevCon;			// 用于设置管线状态, 将资源绑定到图形管线和生成渲染命令
ID3D11RenderTargetView* renderTargetView;	// 渲染目标视图(资源不能直接绑定到一个管线阶段, 必须为资源创建资源视图, 然后绑定到不同的管线阶段)

ID3D11Buffer* squareVertBuffer;
ID3D11Buffer* squareIndexBuffer;
ID3D11VertexShader* VS;
ID3D11PixelShader* PS;
ID3D10Blob* VS_Buffer; // hold the information about vertex shader, use this buffer to create the actual shader
ID3D10Blob* PS_Buffer; // hold the information about pixel shader, use this buffer to create the actual shader
ID3D11InputLayout* vertLayout;

LPCTSTR WndClassName = L"firstwindow";
HWND hwnd = NULL;
HRESULT hr; // use hr for error checking


const int Width = 800;
const int Height = 800;
bool enable4xMsaa = true;

//Function Prototypes//
bool InitializeDirect3d11App(HINSTANCE hInstance);	// initialize direct3d
void ReleaseObjects();	// release the objects we don't need to prevent memory leaks
bool InitScene(); // set up
void UpdateScene(); // change scene on a per-frame basis
void DrawScene(); // draw scene to the screen, update every frame
bool InitializeWindow(HINSTANCE hInstance,
					  int ShowWnd,
					  int width, int height,
					  bool windowed);
int messageloop();
LRESULT CALLBACK WndProc(HWND hWnd,
						 UINT msg,
						 WPARAM wParam,
						 LPARAM lParam);

// Vertex Structure and Vertex Layout
struct Vertex {
	Vertex() {}
	Vertex(float x, float y, float z,
		   float r, float g, float b, float a)
		: pos(x, y, z), color(r, g, b, a)
	{
	}

	XMFLOAT3 pos;
	XMFLOAT4 color;
};

D3D11_INPUT_ELEMENT_DESC layout[] = {
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
};
UINT numElements = ARRAYSIZE(layout); // hold the size of our input layout array

int WINAPI WinMain(HINSTANCE hInstance,	//Main windows function
				   HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine,
				   int nShowCmd)
{
	if(!InitializeWindow(hInstance, nShowCmd, Width, Height, true))
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
	wc.lpszClassName = WndClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if(!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Error registering class",
				   L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	hwnd = CreateWindowEx(
		NULL,
		WndClassName,
		L"Indices",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		width, height,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if(!hwnd)
	{
		MessageBox(NULL, L"Error creating window",
				   L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	ShowWindow(hwnd, ShowWnd);
	UpdateWindow(hwnd);

	return true;
}

bool InitializeDirect3d11App(HINSTANCE hInstance)
{
	// Create Device and Context (+ DX11 龙书中译 P63)
	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel;
	hr = D3D11CreateDevice(
		0,
		D3D_DRIVER_TYPE_HARDWARE,
		0,
		createDeviceFlags,
		0, 0,
		D3D11_SDK_VERSION,
		&d3d11Device,
		&featureLevel,
		&d3d11DevCon);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
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
	d3d11Device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMsaaQuality); // 如果纹理格式和采样数量的组合不被设备支持, 则该方法返回 0
	assert(m4xMsaaQuality > 0);

	// Describe our back buffer
	DXGI_MODE_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));

	bufferDesc.Width = Width;
	bufferDesc.Height = Height;
	bufferDesc.RefreshRate.Numerator = 60;
	bufferDesc.RefreshRate.Denominator = 1;
	bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; // describe the manner in which the rasterizer will render onto a surface
	bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; // explain how an image is stretched to fit a monitors resolution

	// Describe our SwapChain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapChainDesc.BufferDesc = bufferDesc; // 描述所要创建的后台缓冲区的属性
	// 是否使用 4X MSAA (+ DX11 龙书中译 P67)
	if(enable4xMsaa)
	{
		swapChainDesc.SampleDesc.Count = 4; // 多重采样数量
		swapChainDesc.SampleDesc.Quality = m4xMsaaQuality - 1; // 多重采样质量级别
	}
	else
	{
		swapChainDesc.SampleDesc.Count = 1;   // 多重采样数量
		swapChainDesc.SampleDesc.Quality = 0; // 多重采样质量级别
	}
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 将此缓冲用作渲染目标
	swapChainDesc.BufferCount = 1;     // 交换链中后台缓冲的数量
	swapChainDesc.OutputWindow = hwnd; // 要渲染到的窗口的句柄
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // 让显卡驱动程序选择最高效的显示模式
	swapChainDesc.Flags = 0;

	// Create our SwapChain (<-> DX11 龙书中译 P68)
	IDXGIDevice * dxgiDevice = 0;
	hr = d3d11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("d3d11Device->QueryInterface"), MB_OK);
		return 0;
	}
	IDXGIAdapter* dxgiAdapter = 0;
	hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("dxgiDevice->GetParent"), MB_OK);
		return 0;
	}
	// 获得 IDXGIFactory 接口
	IDXGIFactory* dxgiFactory = 0;
	hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("dxgiAdapter->GetParent"), MB_OK);
		return 0;
	}
	// 现在，创建交换链
	dxgiFactory->CreateSwapChain(d3d11Device, &swapChainDesc, &SwapChain);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("dxgiFactory->CreateSwapChain"), MB_OK);
		return 0;
	}
	// 释放 COM 接口
	dxgiDevice->Release();
	dxgiAdapter->Release();
	dxgiFactory->Release();

	// 为后台缓冲区创建一个渲染目标视图 (<-> DX11 龙书中译 P69)
	ID3D11Texture2D* backBuffer;
	hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)); // 获取一个交换链的后台缓冲区指针
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("SwapChain->GetBuffer"), MB_OK);
		return 0;
	}
	hr = d3d11Device->CreateRenderTargetView(backBuffer, 0, &renderTargetView); // 创建渲染目标视图
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("d3d11Device->CreateRenderTargetView"), MB_OK);
		return 0;
	}
	backBuffer->Release(); // now we don't need the backbuffer anymore, we can release it

	// 将视图绑定到输出合并阶段 Output Merger Stage (<-> DX11 龙书中译 P71)
	d3d11DevCon->OMSetRenderTargets(1, &renderTargetView, 0);

	return true;
}

void ReleaseObjects()
{
	//Release the COM Objects we created
	SwapChain->Release();
	d3d11Device->Release();
	d3d11DevCon->Release();
	renderTargetView->Release();
	squareVertBuffer->Release();
	squareIndexBuffer->Release();
	VS->Release();
	PS->Release();
	VS_Buffer->Release();
	PS_Buffer->Release();
	vertLayout->Release();
}

bool InitScene()
{
	// Compile Shader from shader file
	hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "VS", "vs_5_0", 0, 0, 0, &VS_Buffer, 0, 0);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("D3DX11CompileFromFile"), MB_OK);
		return 0;
	}
	hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "PS", "ps_5_0", 0, 0, 0, &PS_Buffer, 0, 0);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("D3DX11CompileFromFile"), MB_OK);
		return 0;
	}
	// Create the Shader Objects
	hr = d3d11Device->CreateVertexShader(VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), NULL, &VS);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("d3d11Device->CreateVertexShader"), MB_OK);
		return 0;
	}
	hr = d3d11Device->CreatePixelShader(PS_Buffer->GetBufferPointer(), PS_Buffer->GetBufferSize(), NULL, &PS);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("d3d11Device->CreatePixelShader"), MB_OK);
		return 0;
	}

	// Set Vertex and Pixel Shaders
	d3d11DevCon->VSSetShader(VS, 0, 0);
	d3d11DevCon->PSSetShader(PS, 0, 0);

	Vertex v[] =
	{
		Vertex(-0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f),
		Vertex(-0.5f,  0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f),
		Vertex(0.5f,  0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f),
		Vertex(0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f),
	};

	DWORD indices[] = {
		0, 1, 2,
		0, 2, 3,
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
	hr = d3d11Device->CreateBuffer(&indexBufferDesc, &indexBufferData, &squareIndexBuffer);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("d3d11Device->CreateBuffer"), MB_OK);
		return 0;
	}

	// Set the index buffer
	d3d11DevCon->IASetIndexBuffer(squareIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Create the vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

	vertexBufferDesc.ByteWidth = sizeof(v);
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; // this is a vertex buffer
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;
	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));

	vertexBufferData.pSysMem = v;
	hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &squareVertBuffer);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("d3d11Device->CreateBuffer"), MB_OK);
		return 0;
	}

	// Set the vertex buffer
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	d3d11DevCon->IASetVertexBuffers(0, 1, &squareVertBuffer, &stride, &offset);

	// Create the Input Layout
	hr = d3d11Device->CreateInputLayout(layout, numElements, VS_Buffer->GetBufferPointer(),
										VS_Buffer->GetBufferSize(), &vertLayout);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("d3d11Device->CreateInputLayout"), MB_OK);
		return 0;
	}

	// Set the Input Layout
	d3d11DevCon->IASetInputLayout(vertLayout);

	// Set Primitive Topology
	d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the Viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(viewport));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Width;
	viewport.Height = Height;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

	// Set the Viewport
	d3d11DevCon->RSSetViewports(1, &viewport);

	return true;
}

void UpdateScene()
{

}

void DrawScene()
{
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	d3d11DevCon->ClearRenderTargetView(renderTargetView, bgColor);

	d3d11DevCon->DrawIndexed(6, 0, 0);

	SwapChain->Present(0, 0);
}

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
