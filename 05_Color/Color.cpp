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
ID3D11Device* d3d11Device;					// ���ڼ����ʾ���������ܺͷ�����Դ
ID3D11DeviceContext* d3d11DevCon;			// �������ù���״̬, ����Դ�󶨵�ͼ�ι��ߺ�������Ⱦ����
ID3D11RenderTargetView* renderTargetView;	// ��ȾĿ����ͼ(��Դ����ֱ�Ӱ󶨵�һ�����߽׶�, ����Ϊ��Դ������Դ��ͼ, Ȼ��󶨵���ͬ�Ĺ��߽׶�)

ID3D11Buffer* triangleVertBuffer;
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
		   float r, float g, float b, float a,
		   float rr, float gg, float bb, float aa)
		: pos(x, y, z), color(r, g, b, a), color2(rr, gg, aa, bb)
	{
	}

	XMFLOAT3 pos;
	XMFLOAT4 color;
	XMFLOAT4 color2;
};

// Instead of naming the 2nd color COLOR2 I changed the names in COLOR_ZERO and COLOR_ONE. 
// A name containing a number results in an error on d3d11Device->CreateInputLayout. 
// Without any numbers all works fine.
D3D11_INPUT_ELEMENT_DESC layout[] = {
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"COLOR_ZERO", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"COLOR_ONE", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
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
		L"Color",
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
	// Create Device and Context (+ DX11 �������� P63)
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

	// Check MSAA (+ DX11 �������� P65)
	UINT m4xMsaaQuality;
	d3d11Device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMsaaQuality); // ��������ʽ�Ͳ�����������ϲ����豸֧��, ��÷������� 0
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

	swapChainDesc.BufferDesc = bufferDesc; // ������Ҫ�����ĺ�̨������������
	// �Ƿ�ʹ�� 4X MSAA (+ DX11 �������� P67)
	if(enable4xMsaa)
	{
		swapChainDesc.SampleDesc.Count = 4; // ���ز�������
		swapChainDesc.SampleDesc.Quality = m4xMsaaQuality - 1; // ���ز�����������
	}
	else
	{
		swapChainDesc.SampleDesc.Count = 1;   // ���ز�������
		swapChainDesc.SampleDesc.Quality = 0; // ���ز�����������
	}
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // ���˻���������ȾĿ��
	swapChainDesc.BufferCount = 1;     // �������к�̨���������
	swapChainDesc.OutputWindow = hwnd; // Ҫ��Ⱦ���Ĵ��ڵľ��
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // ���Կ���������ѡ�����Ч����ʾģʽ
	swapChainDesc.Flags = 0;

	// Create our SwapChain (<-> DX11 �������� P68)
	IDXGIDevice * dxgiDevice = 0;
	hr = d3d11Device->QueryInterface(__uuidof(IDXGIDevice),(void**)&dxgiDevice);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("d3d11Device->QueryInterface"), MB_OK);
		return 0;
	}
	IDXGIAdapter* dxgiAdapter = 0;
	hr= dxgiDevice->GetParent(__uuidof(IDXGIAdapter),(void**) &dxgiAdapter);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("dxgiDevice->GetParent"), MB_OK);
		return 0;
	}
	// ��� IDXGIFactory �ӿ�
	IDXGIFactory* dxgiFactory = 0;
	hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("dxgiAdapter->GetParent"), MB_OK);
		return 0;
	}
	// ���ڣ�����������
	dxgiFactory->CreateSwapChain(d3d11Device, &swapChainDesc, &SwapChain);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("dxgiFactory->CreateSwapChain"), MB_OK);
		return 0;
	}
	// �ͷ� COM �ӿ�
	dxgiDevice->Release();
	dxgiAdapter->Release();
	dxgiFactory->Release();

	// Ϊ��̨����������һ����ȾĿ����ͼ (<-> DX11 �������� P69)
	ID3D11Texture2D* backBuffer; 
	hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)); // ��ȡһ���������ĺ�̨������ָ��
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("SwapChain->GetBuffer"), MB_OK);
		return 0;
	}
	hr = d3d11Device->CreateRenderTargetView(backBuffer, 0, &renderTargetView); // ������ȾĿ����ͼ
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("d3d11Device->CreateRenderTargetView"), MB_OK);
		return 0;
	}
	backBuffer->Release(); // now we don't need the backbuffer anymore, we can release it

	// ����ͼ�󶨵�����ϲ��׶� Output Merger Stage (<-> DX11 �������� P71)
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
	triangleVertBuffer->Release();
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

	// Create the vertex buffer, D3D11_BUFFER_DESC + D3D11_SUBRESOURCE_DATA -> ID3D11Buffer 
	//Vertex v[] =
	//{
	//	Vertex(0.0f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f),
	//	Vertex(0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f),
	//	Vertex(-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f),
	//};

	Vertex v[] =
	{
		Vertex(0.0f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.5f),
		Vertex(0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.5f),
		Vertex(-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.5f),
	};

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

	vertexBufferDesc.ByteWidth = sizeof(v);
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;  // this is a vertex buffer
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;
	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));

	vertexBufferData.pSysMem = v;

	hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &triangleVertBuffer);
	if(FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
				   TEXT("d3d11Device->CreateBuffer"), MB_OK);
		return 0;
	}

	// Set the vertex buffer
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	d3d11DevCon->IASetVertexBuffers(0, 1, &triangleVertBuffer, &stride, &offset);

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

	d3d11DevCon->Draw(3, 0);

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
