/****************************************************************************
Copyright (c) 2013 cocos2d-x.org
Copyright (c) Microsoft Open Technologies, Inc.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/
#include "Direct3DInterop.h"
#include "Direct3DContentProvider.h"
#include "EditBoxEvent.h"
#include "cocos2d.h"
#include "CustomRenderer.h"

using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Microsoft::WRL;
using namespace Windows::Phone::Graphics::Interop;
using namespace Windows::Phone::Input::Interop;
using namespace Windows::Graphics::Display;
using namespace DirectX;
using namespace PhoneDirect3DXamlAppComponent;

namespace PhoneDirect3DXamlAppComponent
{

Direct3DInterop::Direct3DInterop() 
    : m_delegate(nullptr)
{
}

IDrawingSurfaceBackgroundContentProvider^ Direct3DInterop::CreateContentProvider()
{
	ComPtr<Direct3DContentProvider> provider = Make<Direct3DContentProvider>(this);
	return reinterpret_cast<IDrawingSurfaceBackgroundContentProvider^>(provider.Get());
}

// IDrawingSurfaceManipulationHandler
void Direct3DInterop::SetManipulationHost(DrawingSurfaceManipulationHost^ manipulationHost)
{
    manipulationHost->PointerPressed +=
        ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Direct3DInterop::OnPointerPressed);

    manipulationHost->PointerMoved +=
        ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Direct3DInterop::OnPointerMoved);

    manipulationHost->PointerReleased +=
        ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Direct3DInterop::OnPointerReleased);
}

void Direct3DInterop::UpdateForWindowSizeChange(float width, float height)
{
    m_renderer->UpdateForWindowSizeChange(width, height);
}


IAsyncAction^ Direct3DInterop::OnSuspending()
{
    return m_renderer->OnSuspending();
}

void Direct3DInterop::OnBackKeyPress()
{
	cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(new cocos2d::EventKeyboard(cocos2d::EventKeyboard::KeyCode::KEY_BACK, true));
}

// Pointer Event Handlers. We need to queue up pointer events to pass them to the drawing thread
void Direct3DInterop::OnPointerPressed(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
    cocos2d::GLView::sharedOpenGLView()->QueuePointerEvent(cocos2d::PointerEventType::PointerPressed, args);
}

void Direct3DInterop::OnPointerMoved(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
    cocos2d::GLView::sharedOpenGLView()->QueuePointerEvent(cocos2d::PointerEventType::PointerMoved, args);
}

void Direct3DInterop::OnPointerReleased(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
    cocos2d::GLView::sharedOpenGLView()->QueuePointerEvent(cocos2d::PointerEventType::PointerReleased, args);
}

void Direct3DInterop::OnCocos2dKeyEvent(Cocos2dKeyEvent key)
{
    std::shared_ptr<cocos2d::InputEvent> e(new cocos2d::KeyboardEvent(key));
    cocos2d::GLView::sharedOpenGLView()->QueueEvent(e);
}


void Direct3DInterop::OnCocos2dKeyEvent(Cocos2dKeyEvent key, Platform::String^ text)
{
    std::shared_ptr<cocos2d::InputEvent> e(new cocos2d::KeyboardEvent(key,text));
    cocos2d::GLView::sharedOpenGLView()->QueueEvent(e);
}


void Direct3DInterop::OnCocos2dEditboxEvent(Object^ sender, Platform::String^ args, Windows::Foundation::EventHandler<Platform::String^>^ handler)
{
	std::shared_ptr<cocos2d::InputEvent> e(new EditBoxEvent(sender, args, handler));
    cocos2d::GLView::sharedOpenGLView()->QueueEvent(e);
}

// Interface With Direct3DContentProvider
HRESULT Direct3DInterop::Connect(_In_ IDrawingSurfaceRuntimeHostNative* host)
{
	UNREFERENCED_PARAMETER(host);

	if (!m_renderer)
	m_renderer = ref new CustomRenderer();
	m_renderer->Initialize();
	m_renderer->UpdateForWindowSizeChange(WindowBounds.Width, WindowBounds.Height);
	m_renderer->UpdateForRenderResolutionChange(m_renderResolution.Width, m_renderResolution.Height);

	return S_OK;
}

void Direct3DInterop::Disconnect()
{
	m_renderer->Disconnect();
	//m_renderer = nullptr;
}

HRESULT Direct3DInterop::PrepareResources(_In_ const LARGE_INTEGER* presentTargetTime, _Out_ BOOL* contentDirty)
{
	UNREFERENCED_PARAMETER(presentTargetTime);

	*contentDirty = true;

	return S_OK;
}

HRESULT Direct3DInterop::GetTexture(_In_ const DrawingSurfaceSizeF* size, _Inout_ IDrawingSurfaceSynchronizedTextureNative** synchronizedTexture, _Inout_ DrawingSurfaceRectF* textureSubRectangle)
    {
	UNREFERENCED_PARAMETER(size);
	UNREFERENCED_PARAMETER(synchronizedTexture);
	UNREFERENCED_PARAMETER(textureSubRectangle);

    m_renderer->Render();
	RequestAdditionalFrame();
	return S_OK;
}

ID3D11Texture2D* Direct3DInterop::GetTexture()
{
	return m_renderer->GetTexture();
}

void Direct3DInterop::SetCocos2dEventDelegate(Cocos2dEventDelegate^ delegate) 
{ 
    m_delegate = delegate; 
    m_renderer->SetXamlEventDelegate(delegate);
}

void Direct3DInterop::SetCocos2dMessageBoxDelegate(Cocos2dMessageBoxDelegate ^ delegate)
{
    m_messageBoxDelegate = delegate;
    m_renderer->SetXamlMessageBoxDelegate(delegate);
}

void Direct3DInterop::SetCocos2dEditBoxDelegate(Cocos2dEditBoxDelegate ^ delegate)
{
    m_editBoxDelegate = delegate;
    m_renderer->SetXamlEditBoxDelegate(delegate);
}


bool Direct3DInterop::SendCocos2dEvent(Cocos2dEvent event)
{
    if(m_delegate)
    {
        m_delegate->Invoke(event);
        return true;
    }
    return false;
}

void Direct3DInterop::RenderResolution::set(Windows::Foundation::Size renderResolution)
{
	if (renderResolution.Width != m_renderResolution.Width ||
		renderResolution.Height != m_renderResolution.Height)
	{
		m_renderResolution = renderResolution;

		if (m_renderer)
		{
			m_renderer->UpdateForRenderResolutionChange(m_renderResolution.Width, m_renderResolution.Height);
			RecreateSynchronizedTexture();
		}
	}
}

}
