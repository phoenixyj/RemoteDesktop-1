#include "stdafx.h"
#include "Display.h"
#include "Image.h"
#include "CommonNetwork.h"

RemoteDesktop::Display::Display(HWND hwnd) : _HWND(hwnd){

	_System_Cursors = GetSystemCursors();

	CURSORINFO cinfo;
	cinfo.cbSize = sizeof(cinfo);
	if (!GetCursorInfo(&cinfo)){

		auto f = std::find_if(_System_Cursors.begin(), _System_Cursors.end(), [&](const Cursor_Type& a){
			return cinfo.hCursor == a.HCursor;
		});
		if (f != _System_Cursors.end()){
			HCursor = *f;
		}
		else DEBUG_MSG("Could find the mouse on init!");
	}
}

void _Draw(HDC hdc, HDC memhdc, HBITMAP bmp, int width, int height){
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(memhdc, bmp);
	BitBlt(hdc, 0, 0, width, height, memhdc, 0, 0, SRCCOPY);
	SelectObject(memhdc, hOldBitmap);
}

void RemoteDesktop::Display::Draw(HDC hdc){
	auto ptr = _HBITMAP_wrapper.get();
	if (ptr == nullptr) return;
	RECT rect;
	if (!GetClientRect(_HWND, &rect)) return;
	if (rect.bottom == 0 && rect.left == 0 && rect.right == 0 && rect.top == 0) {
		DEBUG_MSG("Exiting cannot see window");
		return;
	}

	POINT p;
	if (!GetCursorPos(&p)) {
		DEBUG_MSG("Exiting cannot GetCursorPos");
		return;
	}
	bool inwindow = p.x > rect.left && p.x < rect.right && p.y> rect.top && p.y < rect.bottom;

	std::lock_guard<std::mutex> lock(_DrawLock);
	auto hMemDC = CreateCompatibleDC(hdc);
	_Draw(hdc, hMemDC, ptr->Bitmap, ptr->width, ptr->height);
	if (inwindow && (GetFocus() == _HWND)) {
		SetCursor(HCursor.HCursor);
	}
	else {
		DrawIcon(hdc, _MousePos.left, _MousePos.top, HCursor.HCursor);
	}
	DeleteDC(hMemDC);
}

void RemoteDesktop::Display::NewImage(Image& img){
	BITMAPINFO   bi;

	bi.bmiHeader.biSize = sizeof(bi);
	bi.bmiHeader.biWidth = img.width;
	bi.bmiHeader.biHeight = -img.height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSizeImage = ((img.width * bi.bmiHeader.biBitCount + 31) / 32) * 4 * img.height;

	std::lock_guard<std::mutex> lock(_DrawLock);

	auto hDC = GetDC(_HWND);
	void* raw_data = nullptr;
	_HBITMAP_wrapper = std::make_unique<HBITMAP_wrapper>(CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &raw_data, NULL, NULL));
	_HBITMAP_wrapper->height = img.height;
	_HBITMAP_wrapper->width = img.width;
	_HBITMAP_wrapper->raw_data = (unsigned char*)raw_data;

	memcpy(raw_data, img.data, img.size_in_bytes);
	ReleaseDC(_HWND, hDC);
}
void RemoteDesktop::Display::UpdateImage(Image& img, Image_Diff_Header& h){
	auto ptr = _HBITMAP_wrapper.get();
	if (ptr != nullptr) {
		std::lock_guard<std::mutex> lock(_DrawLock);
		Image::Copy(img, h.rect.left, h.rect.top, ptr->width * 4, ptr->raw_data);
	}
	if (ptr != nullptr) InvalidateRect(_HWND, NULL, false);
}
void RemoteDesktop::Display::UpdateMouse(MouseEvent_Header& h){
	_MousePos = h.pos;
	DEBUG_MSG("Mouse update to   %  %", h.pos.left, h.pos.top);

	CURSORINFO cinfo;
	cinfo.cbSize = sizeof(cinfo);
	if (!GetCursorInfo(&cinfo)){
		DEBUG_MSG("Exiting cannot GetCursorInfo");
		return;
	}

	auto f = std::find_if(_System_Cursors.begin(), _System_Cursors.end(), [&](const Cursor_Type& a){
		return h.HandleID == a.ID;
	});

	if (f != _System_Cursors.end()){
		HCursor = *f;
		if (f->ID != HCursor.ID){
			InvalidateRect(_HWND, NULL, false);
		}
	}
}
