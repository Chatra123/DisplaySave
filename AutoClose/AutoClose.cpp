#pragma once
#include <windows.h>
#include <string>
#include <chrono>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
using namespace std;
using namespace std::chrono;


// プラグインクラス
class CAutoClose : public TVTest::CTVTestPlugin
{
  const bool EnableLog = false;//  true  false
  bool m_fInitialized;			// 初期化済みか?
  HWND m_hwnd;						  // ウィンドウハンドル
  bool m_fEnabled;					// プラグインが有効か？
  time_point<system_clock> m_LastTime_TVTestEvent, m_LastTime_UserInput;

  bool InitializePlugin();
  bool OnEnablePlugin(bool fEnable);
  void AutoClose();
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
  static BOOL CALLBACK MessageCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    LRESULT *pResult, void *pClientData);
  static CAutoClose *GetThis(HWND hwnd);

private:
  static const  wstring WINDOW_CLASS_NAME;
  enum {
    TIMER_ID = 1,
  };

public:
  CAutoClose();
  virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
  virtual bool Initialize();
  virtual bool Finalize();
};
const wstring CAutoClose::WINDOW_CLASS_NAME = L"TVTest AutoClose Window";


CAutoClose::CAutoClose()
  : m_fInitialized(false)
  , m_hwnd(nullptr)
  , m_fEnabled(false)
{
}


bool CAutoClose::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
  // プラグインの情報を返す
  pInfo->Type = TVTest::PLUGIN_TYPE_NORMAL;
  pInfo->pszPluginName = L"AutoClose";
  pInfo->pszCopyright = L"";
  pInfo->pszDescription = L"TVTestの自動終了";
  return true;
}


// 初期化処理
bool CAutoClose::Initialize()
{
  // イベントコールバック関数を登録
  m_pApp->SetEventCallback(EventCallback, this);
  m_pApp->SetWindowMessageCallback(MessageCallback, this);
  return true;
}


// プラグインが有効にされた時の初期化処理
bool CAutoClose::InitializePlugin()
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
bool CAutoClose::Finalize()
{
  if (m_hwnd)
    ::DestroyWindow(m_hwnd);
  ::KillTimer(m_hwnd, TIMER_ID);
  return true;
}


// プラグインの有効/無効が切り替わった時の処理
bool CAutoClose::OnEnablePlugin(bool fEnable)
{
  InitializePlugin();

  m_fEnabled = fEnable;
  if (m_fEnabled)
  {
    m_LastTime_TVTestEvent = system_clock::now();
    m_LastTime_UserInput = system_clock::now();

    const int sec = 5 * 60;//  2  5
    ::SetTimer(m_hwnd, TIMER_ID, sec * 1000, nullptr);
  }
  else
  {
    ::KillTimer(m_hwnd, TIMER_ID);
  }
  return true;
}



// 自動終了の判定
void CAutoClose::AutoClose()
{
  const int TimeLimit = 150;
  const int TimeLimit_withoutStream = 20;

  // 録画中？
  bool recNow;
  {
    TVTest::RecordStatusInfo recInfo;
    if (m_pApp->GetRecordStatus(&recInfo))
      recNow =
      recInfo.Status == TVTest::RECORD_STATUS_RECORDING ||
      recInfo.Status == TVTest::RECORD_STATUS_PAUSED;
    else
      recNow = true;//取得失敗
  }

  // VideoStream ?
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

  // user control ?
  bool hasUser;
  double user_elapse;
  {
    user_elapse = duration_cast<minutes>(system_clock::now() - m_LastTime_UserInput).count();
    if (hasStream)
      hasUser = user_elapse < TimeLimit;
    else
      hasUser = user_elapse < TimeLimit_withoutStream;
  }

  if (EnableLog)
  {
    wstring log = L"";
    log = L""; m_pApp->AddLog(log.c_str());
    log = L"recNow    = " + to_wstring(recNow); m_pApp->AddLog(log.c_str());
    log = L"hasStream = " + to_wstring(hasStream); m_pApp->AddLog(log.c_str());
    log = L"hasUser   = " + to_wstring(hasUser); m_pApp->AddLog(log.c_str());
    log = L"  elapse  = " + to_wstring((int)user_elapse); m_pApp->AddLog(log.c_str());
  }

  if (recNow == false &&  hasUser == false)
  {
    m_pApp->AddLog(L"------------------------");
    m_pApp->AddLog(L"Close TVTest");
    m_pApp->AddLog(L"------------------------");
    m_pApp->Close();
  }
  return;
}



// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CAutoClose::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
  CAutoClose *pThis = static_cast<CAutoClose*>(pClientData);
  switch (Event) {
  case TVTest::EVENT_PLUGINENABLE:
    // プラグインの有効状態が変化した
    return pThis->OnEnablePlugin(lParam1 != 0);
  }
  return 0;
}


// メッセージコールバック関数
// TVTestウィンドウのマウス、キ－ボード操作を検出
// 画面のみでタイトルバー、ステータスバーでの操作は検出できない
BOOL CALLBACK CAutoClose::MessageCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
  LRESULT *pResult, void *pClientData)
{
  switch (uMsg) {
  case WM_LBUTTONDOWN:
  case WM_RBUTTONDOWN:
  case WM_MOUSEWHEEL:
  //case WM_MOUSEMOVE:
  case WM_KEYDOWN:
  {
    CAutoClose *pThis = static_cast<CAutoClose*>(pClientData);
    pThis->m_LastTime_UserInput = system_clock::now();
    if (pThis->EnableLog)
    {
      if (uMsg == WM_LBUTTONDOWN)
        pThis->m_pApp->AddLog(L"WM_LBUTTONDOWN");
      else if (uMsg == WM_RBUTTONDOWN)
        pThis->m_pApp->AddLog(L"WM_RBUTTONDOWN");
      else if (uMsg == WM_MOUSEWHEEL)
        pThis->m_pApp->AddLog(L"WM_MOUSEWHEEL");
      else if (uMsg == WM_KEYDOWN)
        pThis->m_pApp->AddLog(L"WM_KEYDOWN");
    }
  }
  return FALSE;
  }
  return FALSE;
}



// ウィンドウプロシージャ関数
// タイマーを処理
LRESULT CALLBACK CAutoClose::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
  case WM_CREATE:
  {
    LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
    CAutoClose *pThis = static_cast<CAutoClose*>(pcs->lpCreateParams);
    ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
  }
  return 0;
  case WM_TIMER:
  {
    CAutoClose *pThis = GetThis(hwnd);
    pThis->AutoClose();
  }
  return 0;
  }
  return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


// ウィンドウハンドルからthisを取得する
CAutoClose *CAutoClose::GetThis(HWND hwnd)
{
  return reinterpret_cast<CAutoClose*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin *CreatePluginClass()
{
  return new CAutoClose;
}

