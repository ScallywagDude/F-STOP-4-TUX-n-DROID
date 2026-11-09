// Compile production menu methods against small VGUI doubles; SDK/device checks are separate.
const fs = require('node:fs');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
const root = path.resolve(__dirname, '../..');
const dir = path.join(root, 'out/controller-validation');
fs.mkdirSync(path.join(dir, 'mathlib'), { recursive: true });
fs.mkdirSync(path.join(dir, 'inputsystem'), { recursive: true });
fs.writeFileSync(path.join(dir, 'mathlib/mathlib.h'), 'template<class T> T clamp(T x,T a,T b){return x<a?a:x>b?b:x;}\n');
for (const file of ['ButtonCode.h', 'InputEnums.h']) {
  fs.copyFileSync(path.join(root, 'engine/public/inputsystem', file), path.join(dir, 'inputsystem', file));
}
function method(file, name) {
  const source = fs.readFileSync(path.join(root, 'engine/game/client/portal2/basemodui', file), 'latin1');
  const start = source.indexOf('void ' + name + '(');
  if (start < 0) throw new Error('Missing production method: ' + name);
  let end = source.indexOf('{', start), depth = 1;
  while (depth && ++end < source.length) {
    if (source[end] === '{') ++depth;
    if (source[end] === '}') --depth;
  }
  if (depth) throw new Error('Unclosed production method: ' + name);
  return source.slice(start, end + 1);
}
const preamble = String.raw`
#include <cassert>
#include <climits>
#include <iostream>
#include <set>
#include <vector>
#include "inputsystem/ButtonCode.h"
using KeyCode=ButtonCode_t;
struct Panel;using VPANEL=Panel*;
static VPANEL requestedFocus=nullptr;static bool deferFocus=false;static VPANEL focus=nullptr;static float clockTime=0;static bool menuVisible=true;
float Plat_FloatTime(){return clockTime;}bool IsPC(){return true;}
#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))
enum{WT_NONE,WT_MAINMENU,WT_INGAMEMAINMENU,WT_OPTIONS,WT_AUDIO};
struct Input{std::set<KeyCode> keys;VPANEL GetFocus(){return focus;}bool IsKeyDown(KeyCode k){return keys.count(k)>0;}}theInput;
Input*input(){return &theInput;}
struct Panel{
 virtual ~Panel(){}bool visible=true,enabled=true,keyboard=true;int tab=0,x=0,y=0,navs=0,baseKeys=0;Panel*parent=nullptr,*up=nullptr,*down=nullptr;std::vector<Panel*>children;
 bool IsVisible(){return visible;}bool IsEnabled(){return enabled;}bool IsKeyBoardInputEnabled(){return keyboard;}VPANEL GetVPanel(){return this;}int GetTabPosition(){return tab;}
 int GetChildCount(){return (int)children.size();}Panel*GetChild(int i){return children.at(i);}void GetPos(int&a,int&b){a=x;b=y;}void SetNavUp(Panel*p){up=p;}void SetNavDown(Panel*p){down=p;}
 virtual VPANEL GetCurrentKeyFocus(){return nullptr;}virtual void NavigateTo(){navs++;}virtual void RequestFocus(){requestedFocus=this;if(!deferFocus)focus=this;}virtual void OnSetFocus(){}virtual void OnKeyCodePressed(KeyCode){baseKeys++;}void Add(Panel*p){children.push_back(p);p->parent=this;}
};
struct Button:Panel{};
struct PanelAPI{bool HasParent(VPANEL p,VPANEL a){for(;p;p=p->parent)if(p==a)return true;return false;}}theAPI;
PanelAPI*ipanel(){return &theAPI;}
template<class T>struct CUtlVector:std::vector<T>{int Count(){return (int)this->size();}void InsertBefore(int i,T v){this->insert(this->begin()+i,v);}};
struct CBaseModFrame:Panel{Panel*m_ActiveControl=nullptr;Panel*GetActiveControl()const{return m_ActiveControl;}int type=WT_MAINMENU;int GetWindowType(){return type;}VPANEL GetCurrentKeyFocus(){return ipanel()->HasParent(focus,this)?focus:nullptr;}void FindAndSetActiveControl();void RestoreFocusToActiveControl();void ConfigureMenuNavigation();};
struct KeyValues{int code;KeyValues(const char*,const char*,int k):code(k){}};
struct Message{VPANEL target;int code;};
struct EngineVGui{bool IsGameUIVisible(){return menuVisible;}}theEngineVGui;
EngineVGui*enginevgui=&theEngineVGui;
struct CBaseModPanel:Panel{
 using BaseClass=Panel;CBaseModFrame*frame=nullptr;bool active=false;int user=0;KeyCode m_RepeatKey=BUTTON_CODE_NONE;float m_flNextControllerRepeat=0;std::vector<Message>messages;
 CBaseModFrame*GetWindow(int){return frame;}int GetActiveWindowType(){return frame?frame->type:WT_NONE;}bool IsControllerActive(){return active;}void SetControllerActive(bool a){active=a;}int GetLastActiveUserId(){return user;}void SetLastActiveUserId(int u){user=u;}
 void PostMessage(VPANEL p,KeyValues*k){messages.push_back({p,k->code});delete k;}void OnMenuKeyPressed(KeyCode);void OnSetFocus();void OnKeyCodePressed(KeyCode);void UpdateControllerKeyRepeat();
};

`;
const checks = String.raw`int main(){
 CBaseModFrame empty;empty.ConfigureMenuNavigation();empty.RestoreFocusToActiveControl();assert(focus==&empty);
 CBaseModFrame f;Button bottom,hidden,disabled,middle,top;bottom.y=100;bottom.tab=3;hidden.y=0;hidden.tab=1;hidden.visible=false;disabled.y=20;disabled.tab=1;disabled.enabled=false;middle.y=50;middle.tab=2;top.y=10;top.tab=1;
 f.Add(&bottom);f.Add(&hidden);f.Add(&disabled);f.Add(&middle);f.Add(&top);f.ConfigureMenuNavigation();assert(top.down==&middle&&middle.down==&bottom&&bottom.down==&top);assert(top.up==&bottom&&middle.up==&top&&bottom.up==&middle);assert(!hidden.up&&!disabled.down);
 focus=nullptr;f.RestoreFocusToActiveControl();assert(f.m_ActiveControl==&top&&focus==&top);focus=&middle;f.FindAndSetActiveControl();assert(f.m_ActiveControl==&middle);focus=nullptr;f.RestoreFocusToActiveControl();assert(focus==&middle);middle.enabled=false;focus=nullptr;f.RestoreFocusToActiveControl();assert(focus==&top);middle.enabled=true;Panel nested;middle.Add(&nested);focus=&nested;f.FindAndSetActiveControl();assert(f.m_ActiveControl==&middle);
 CBaseModFrame audio;audio.type=WT_AUDIO;Button a,b;audio.Add(&a);audio.Add(&b);audio.ConfigureMenuNavigation();assert(!a.down&&!b.up);
 CBaseModPanel r;r.frame=&f;r.Add(&f);focus=&r;r.OnSetFocus();assert(focus==&middle);focus=&r;r.OnKeyCodePressed(KEY_XBUTTON_A);assert(focus==&middle&&r.messages.size()==1&&r.messages.back().target==&middle);r.OnKeyCodePressed(KEY_XBUTTON_DOWN);assert(r.baseKeys==1&&r.messages.size()==1);
 focus=&r;deferFocus=true;r.OnKeyCodePressed(KEY_XBUTTON_A);assert(requestedFocus==&middle&&r.messages.back().target==&middle);deferFocus=false;focus=&middle;
 r.OnMenuKeyPressed(STEAMCONTROLLER_DPAD_DOWN);assert(r.IsControllerActive());r.OnMenuKeyPressed(KEY_ENTER);assert(!r.IsControllerActive());
 r.messages.clear();r.OnMenuKeyPressed(KEY_XBUTTON_DOWN);theInput.keys.insert(KEY_XBUTTON_DOWN);clockTime=0;focus=&top;r.UpdateControllerKeyRepeat();assert(r.messages.empty());clockTime=.39f;r.UpdateControllerKeyRepeat();assert(r.messages.empty());clockTime=.4f;r.UpdateControllerKeyRepeat();assert(r.messages.size()==1&&r.messages.back().target==&top&&r.messages.back().code==KEY_XBUTTON_DOWN);clockTime=.55f;r.UpdateControllerKeyRepeat();assert(r.messages.size()==1);clockTime=.56f;r.UpdateControllerKeyRepeat();assert(r.messages.size()==2);
 theInput.keys.clear();clockTime=1;r.UpdateControllerKeyRepeat();assert(r.messages.size()==2);theInput.keys.insert(KEY_XBUTTON_A);clockTime=2;r.UpdateControllerKeyRepeat();clockTime=4;r.UpdateControllerKeyRepeat();assert(r.messages.size()==2);
 theInput.keys.clear();theInput.keys.insert(KEY_XSTICK1_UP);clockTime=5;r.UpdateControllerKeyRepeat();clockTime=5.5f;r.UpdateControllerKeyRepeat();assert(r.messages.size()==3&&r.messages.back().code==KEY_XSTICK1_UP);menuVisible=false;clockTime=7;r.UpdateControllerKeyRepeat();assert(r.messages.size()==3);menuVisible=true;Panel outside;focus=&outside;clockTime=9;r.UpdateControllerKeyRepeat();assert(r.messages.size()==3);
 std::cout<<"PASS: empty menu; hidden/disabled skipping; sorted wrap; default/restored/nested focus; root routing; device mode; D-pad/stick repeat timing; release/accept/hidden/external-focus guards\n";
}
`;
const methods = [
  ['basemodframe.cpp', 'CBaseModFrame::FindAndSetActiveControl'],
  ['basemodframe.cpp', 'CBaseModFrame::RestoreFocusToActiveControl'],
  ['basemodframe.cpp', 'CBaseModFrame::ConfigureMenuNavigation'],
  ['basemodpanel.cpp', 'CBaseModPanel::OnMenuKeyPressed'],
  ['basemodpanel.cpp', 'CBaseModPanel::OnSetFocus'],
  ['basemodpanel.cpp', 'CBaseModPanel::OnKeyCodePressed'],
  ['basemodpanel.cpp', 'CBaseModPanel::UpdateControllerKeyRepeat'],
].map(([file, name]) => method(file, name)).join('\n');
const cpp = path.join(dir, 'controller_validation.cpp');
const exe = path.join(dir, process.platform === 'win32' ? 'controller_validation.exe' : 'controller_validation');
fs.writeFileSync(cpp, preamble + '\n' + methods + '\n' + checks);
function run(command, args) {
  const result = spawnSync(command, args, { stdio: 'inherit' });
  if (result.error) throw result.error;
  if (result.status !== 0) process.exit(result.status || 1);
}
run(process.env.CXX || 'g++', ['-std=c++11', '-Wall', '-Wextra', '-I', dir, cpp, '-o', exe]);
run(exe, []);
