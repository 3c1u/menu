#include <windows.h>
#include "tp_stub.h"
#include "MenuItemIntf.h"

static std::map<HWND,iTJSDispatch2*> MENU_LIST;
static void AddMenuDispatch( HWND hWnd, iTJSDispatch2* menu ) {
	MENU_LIST.insert( std::map<HWND, iTJSDispatch2*>::value_type( hWnd, menu ) );
}
static iTJSDispatch2* GetMenuDispatch( HWND hWnd ) {
	std::map<HWND, iTJSDispatch2*>::iterator i = MENU_LIST.find( hWnd );
	if( i != MENU_LIST.end() ) {
		return i->second;
	}
	return NULL;
}
static void DelMenuDispatch( HWND hWnd ) {
	MENU_LIST.erase(hWnd);
}
/**
 * ���j���[�̒�������ɑ��݂��Ȃ��Ȃ���Window�ɂ��Ă��郁�j���[�I�u�W�F�N�g���폜����
 */
static void UpdateMenuList() {
	std::map<HWND, iTJSDispatch2*>::iterator i = MENU_LIST.begin();
	for( ; i != MENU_LIST.end(); ) {
		HWND hWnd = i->first;
		BOOL exist = ::IsWindow( hWnd );
		if( exist == 0 ) {
			// ���ɂȂ��Ȃ���Window
			std::map<HWND, iTJSDispatch2*>::iterator target = i;
			i++;
			iTJSDispatch2* menu = target->second;
			MENU_LIST.erase( target );
			menu->Release();
			TVPDeleteAcceleratorKeyTable( hWnd );
		} else {
			i++;
		}
	}
}
class WindowMenuProperty : public tTJSDispatch {
	tjs_error TJS_INTF_METHOD PropGet( tjs_uint32 flag,	const tjs_char * membername, tjs_uint32 *hint, tTJSVariant *result,	iTJSDispatch2 *objthis ) {
		tTJSVariant var;
		if( TJS_FAILED(objthis->PropGet(0, TJS_W("HWND"), NULL, &var, objthis)) ) {
			return TJS_E_INVALIDOBJECT;
		}
		HWND hWnd = (HWND)(tjs_int64)var;
		iTJSDispatch2* menu = GetMenuDispatch( hWnd );
		if( menu == NULL ) {
			UpdateMenuList();
			menu = TVPCreateMenuItemObject(objthis);
			AddMenuDispatch( hWnd, menu );
		}
		*result = tTJSVariant(menu, menu);
		return TJS_S_OK;
	}
	tjs_error TJS_INTF_METHOD PropSet( tjs_uint32 flag, const tjs_char *membername,	tjs_uint32 *hint, const tTJSVariant *param,	iTJSDispatch2 *objthis ) {
		return TJS_E_ACCESSDENYED;
	}
} *gWindowMenuProperty;
//---------------------------------------------------------------------------
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved) {
	return 1;
}
//---------------------------------------------------------------------------
static tjs_int GlobalRefCountAtInit = 0;
extern "C" __declspec(dllexport) HRESULT _stdcall V2Link(iTVPFunctionExporter *exporter)
{
	// �X�^�u�̏�����(�K���L�q����)
	TVPInitImportStub(exporter);

	tTJSVariant val;

	// TJS �̃O���[�o���I�u�W�F�N�g���擾����
	iTJSDispatch2 * global = TVPGetScriptDispatch();

	{
		gWindowMenuProperty = new WindowMenuProperty();
		val = tTJSVariant(gWindowMenuProperty);
		gWindowMenuProperty->Release();
		tTJSVariant win;
		if( TJS_SUCCEEDED(global->PropGet(0,TJS_W("Window"),NULL,&win,global)) ) {
			iTJSDispatch2* obj = win.AsObjectNoAddRef();
			obj->PropSet(TJS_MEMBERENSURE,TJS_W("menu"),NULL,&val,obj);
		}

		//-----------------------------------------------------------------------
		iTJSDispatch2 * tjsclass = TVPCreateNativeClass_MenuItem();
		val = tTJSVariant(tjsclass);
		tjsclass->Release();
		global->PropSet( TJS_MEMBERENSURE, TJS_W("MenuItem"), NULL, &val, global );
		//-----------------------------------------------------------------------
		
	}

	// - global �� Release ����
	global->Release();

	// val ���N���A����B
	// ����͕K���s���B�������Ȃ��� val ���ێ����Ă���I�u�W�F�N�g
	// �� Release ���ꂸ�A���Ɏg�� TVPPluginGlobalRefCount �����m�ɂȂ�Ȃ��B
	val.Clear();


	// ���̎��_�ł� TVPPluginGlobalRefCount �̒l��
	GlobalRefCountAtInit = TVPPluginGlobalRefCount;
	// �Ƃ��čT���Ă����BTVPPluginGlobalRefCount �͂��̃v���O�C������
	// �Ǘ�����Ă��� tTJSDispatch �h���I�u�W�F�N�g�̎Q�ƃJ�E���^�̑��v�ŁA
	// ������ɂ͂���Ɠ������A����������Ȃ��Ȃ��ĂȂ��ƂȂ�Ȃ��B
	// �����Ȃ��ĂȂ���΁A�ǂ����ʂ̂Ƃ���Ŋ֐��Ȃǂ��Q�Ƃ���Ă��āA
	// �v���O�C���͉���ł��Ȃ��ƌ������ƂɂȂ�B

	return S_OK;
}
//---------------------------------------------------------------------------
extern "C" __declspec(dllexport) HRESULT _stdcall V2Unlink()
{
	// �g���g��������A�v���O�C����������悤�Ƃ���Ƃ��ɌĂ΂��֐��B

	// �������炩�̏����Ńv���O�C��������ł��Ȃ��ꍇ��
	// ���̎��_�� E_FAIL ��Ԃ��悤�ɂ���B
	// �����ł́ATVPPluginGlobalRefCount �� GlobalRefCountAtInit ����
	// �傫���Ȃ��Ă���Ύ��s�Ƃ������Ƃɂ���B
	if(TVPPluginGlobalRefCount > GlobalRefCountAtInit) return E_FAIL;
		// E_FAIL ���A��ƁAPlugins.unlink ���\�b�h�͋U��Ԃ�

	/*
		�������A�N���X�̏ꍇ�A�����Ɂu�I�u�W�F�N�g���g�p���ł���v�Ƃ������Ƃ�
		�m�邷�ׂ�����܂���B��{�I�ɂ́APlugins.unlink �ɂ��v���O�C���̉����
		�댯�ł���ƍl���Ă������� (�������� Plugins.link �Ń����N������A�Ō��
		�Ńv���O�C������������A�v���O�����I���Ɠ����Ɏ����I�ɉ��������̂��g)�B
	*/

	// �v���p�e�B�J��
	// - �܂��ATJS �̃O���[�o���I�u�W�F�N�g���擾����
	iTJSDispatch2 * global = TVPGetScriptDispatch();

	// ���j���[�͉������Ȃ��͂��Ȃ̂ŁA�����I�ɂ͉�����Ȃ�

	// - global �� DeleteMember ���\�b�h��p���A�I�u�W�F�N�g���폜����
	if(global)
	{
		// TJS ���̂����ɉ������Ă����Ƃ��Ȃǂ�
		// global �� NULL �ɂȂ蓾��̂� global �� NULL �łȂ�
		// ���Ƃ��`�F�b�N����

		global->DeleteMember( 0, TJS_W("MenuItem"), NULL, global );
	}

	// - global �� Release ����
	if(global) global->Release();

	// �X�^�u�̎g�p�I��(�K���L�q����)
	TVPUninitImportStub();

	return S_OK;
}
//---------------------------------------------------------------------------
