#pragma once
#include <windows.h>
#include <string>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
using namespace std;



// �v���O�C���N���X
class CDisplaySave : public TVTest::CTVTestPlugin
{
  bool m_fInitialized;			// �������ς݂�?
  HWND m_hwnd;						  // �E�B���h�E�n���h��
  bool m_fEnabled;					// �v���O�C�����L�����H

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
  // �v���O�C���̏���Ԃ�
  pInfo->Type = TVTest::PLUGIN_TYPE_NORMAL;
  pInfo->pszPluginName = L"DisplaySave";
  pInfo->pszCopyright = L"";
  pInfo->pszDescription = L"���j�^�[�̏ȓd�͐ݒ�";
  return true;
}


// ����������
bool CDisplaySave::Initialize()
{
  // �C�x���g�R�[���o�b�N�֐���o�^
  m_pApp->SetEventCallback(EventCallback, this);
  return true;
}


// �v���O�C�����L���ɂ��ꂽ���̏���������
bool CDisplaySave::InitializePlugin()
{
  if (m_fInitialized)
    return true;

  // �E�B���h�E�N���X�̓o�^
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
  // �E�B���h�E�̍쐬
  m_hwnd = ::CreateWindowEx(
    0, WINDOW_CLASS_NAME.c_str(), nullptr, WS_POPUP,
    0, 0, 0, 0, HWND_MESSAGE, nullptr, g_hinstDLL, this);
  if (m_hwnd == nullptr)
    return false;

  m_fInitialized = true;
  return true;
}


// �I������
bool CDisplaySave::Finalize()
{
  if (m_hwnd)
    ::DestroyWindow(m_hwnd);
  ::KillTimer(m_hwnd, TIMER_ID);
  return true;
}


// �v���O�C���̗L��/�������؂�ւ�������̏���
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


// ���j�^�[�̏ȓd�͐ݒ������
void CDisplaySave::PreventLowPower(bool fForce)
{
  // VideoStream?
  /*
  �f������~���Ă�������0Mbps�ɂȂ�Ȃ��B5�b���0Mbps�ɂȂ�B
  */
  bool hasStream;
  {
    TVTest::StatusInfo vInfo;
    if (m_pApp->GetStatus(&vInfo))
      hasStream = 100 < vInfo.BitRate;
    else
      hasStream = true;//�擾���s
  }

  //prevent display low power 
  if (fForce || hasStream)
    ::SetThreadExecutionState(ES_DISPLAY_REQUIRED);
}



// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CDisplaySave::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
  CDisplaySave *pThis = static_cast<CDisplaySave*>(pClientData);
  switch (Event) {
  case TVTest::EVENT_PLUGINENABLE:
    // �v���O�C���̗L����Ԃ��ω�����
    return pThis->OnEnablePlugin(lParam1 != 0);
  }
  return 0;
}


// �E�B���h�E�v���V�[�W���֐�
// �^�C�}�[������
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



// �E�B���h�E�n���h������this���擾����
CDisplaySave *CDisplaySave::GetThis(HWND hwnd)
{
  return reinterpret_cast<CDisplaySave*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


// �v���O�C���N���X�̃C���X�^���X�𐶐�����
TVTest::CTVTestPlugin *CreatePluginClass()
{
  return new CDisplaySave;
}
