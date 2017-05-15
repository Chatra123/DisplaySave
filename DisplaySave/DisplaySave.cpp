#pragma once
#include <windows.h>
#include <string>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
using namespace std;



// プラグインクラス
class CDisplaySave : public TVTest::CTVTestPlugin
{
  bool m_fInitialized;			// 初期化済みか?
  HWND m_hwnd;						  // ウィンドウハンドル
  bool m_fEnabled;					// プラグインが有効か？

  bool InitializePlugin();
  bool OnEnablePlugin(bool fEnable);
  void PreventLowPower(bool fForce = false);
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
  static CDisplaySave *GetThis(HWND hwnd);

  static const wstring WINDOW_CLASS_NAME;
  enum {
    TIMER_ID = 1,
  };

public:
  CDisplaySave();
  virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
  virtual bool Initialize();
  virtual bool Finalize();
};
const wstring CDisplaySave::WINDOW_CLASS_NAME = L"TVTest DisplaySave Window";


CDisplaySave::CDisplaySave()
  : m_fInitialized(false)
  , m_hwnd(nullptr)
  , m_fEnabled(false)
{
}


bool CDisplaySave::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
  // プラグインの情報を返す
  pInfo->Type = TVTest::PLUGIN_TYPE_NORMAL;
  pInfo->pszPluginName = L"DisplaySave";
  pInfo->pszCopyright = L"";
  pInfo->pszDescription = L"モニターの省電力設定";
  return true;
}


// 初期化処理
bool CDisplaySave::Initialize()
{
  // イベントコールバック関数を登録
  m_pApp->SetEventCallback(EventCallback, this);
  return true;
}


// プラグインが有効にされた時の初期化処理
bool CDisplaySave::InitializePlugin()
{
  if (m_fInitialized)
    return true;

  // ウィンドウクラスの登録
  WNDCLASS wc;
  wc.style = 0;
  wc.lpfnWndProc = WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = g_hinstDLL;
  wc.hIcon = nullptr;
  wc.hCursor = nullptr;
  wc.hbrBackground = nullptr;
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = WINDOW_CLASS_NAME.c_str();
  if (::RegisterClass(&wc) == 0)
    return false;
  // ウィンドウの作成
  m_hwnd = ::CreateWindowEx(
    0, WINDOW_CLASS_NAME.c_str(), nullptr, WS_POPUP,
    0, 0, 0, 0, HWND_MESSAGE, nullptr, g_hinstDLL, this);
  if (m_hwnd == nullptr)
    return false;

  m_fInitialized = true;
  return true;
}


// 終了処理
bool CDisplaySave::Finalize()
{
  if (m_hwnd)
    ::DestroyWindow(m_hwnd);
  ::KillTimer(m_hwnd, TIMER_ID);
  return true;
}


// プラグインの有効/無効が切り替わった時の処理
bool CDisplaySave::OnEnablePlugin(bool fEnable)
{
  InitializePlugin();

  m_fEnabled = fEnable;
  if (m_fEnabled)
  {
    PreventLowPower(true);
    //const int sec = 30;
    const int sec = 5 * 60;
    ::SetTimer(m_hwnd, TIMER_ID, sec * 1000, nullptr);
  }
  else
    ::KillTimer(m_hwnd, TIMER_ID);
  return true;
}


// モニターの省電力設定を処理
void CDisplaySave::PreventLowPower(bool fForce)
{
  // VideoStream?
  /*
  映像が停止してもすぐに0Mbpsにならない。5秒後に0Mbpsになる。
  */
  bool hasStream;
  {
    TVTest::StatusInfo vInfo;
    if (m_pApp->GetStatus(&vInfo))
      hasStream = 100 < vInfo.BitRate;
    else
      hasStream = true;//取得失敗
  }

  //prevent display low power 
  if (fForce || hasStream)
    ::SetThreadExecutionState(ES_DISPLAY_REQUIRED);
}



// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CDisplaySave::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
  CDisplaySave *pThis = static_cast<CDisplaySave*>(pClientData);
  switch (Event) {
  case TVTest::EVENT_PLUGINENABLE:
    // プラグインの有効状態が変化した
    return pThis->OnEnablePlugin(lParam1 != 0);
  }
  return 0;
}


// ウィンドウプロシージャ関数
// タイマーを処理
LRESULT CALLBACK CDisplaySave::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
  case WM_CREATE:
  {
    LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
    CDisplaySave *pThis = static_cast<CDisplaySave*>(pcs->lpCreateParams);
    ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
  }
  return 0;
  case WM_TIMER:
  {
    CDisplaySave *pThis = GetThis(hwnd);
    pThis->PreventLowPower();
  }
  return 0;
  }
  return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}



// ウィンドウハンドルからthisを取得する
CDisplaySave *CDisplaySave::GetThis(HWND hwnd)
{
  return reinterpret_cast<CDisplaySave*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin *CreatePluginClass()
{
  return new CDisplaySave;
}
