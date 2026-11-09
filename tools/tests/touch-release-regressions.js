// Exercise the production touch state release and event gates with engine doubles.
const fs = require('node:fs');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
const root = path.resolve(__dirname, '../..');
const output = path.join(root, 'out/touch-validation');
fs.mkdirSync(output, { recursive: true });
const source = fs.readFileSync(path.join(root, 'engine/game/client/touch.cpp'), 'latin1');
function method(name) {
  const start = source.indexOf('void CTouchControls::' + name + '(');
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
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#define PORTAL2 1
template<class T> using CUtlLinkedList = std::vector<T>;
void V_strncpy(char* dst,const char* src,size_t size){std::strncpy(dst,src,size);dst[size-1]=0;}
struct CTouchButton{int finger=-1;char command[256]={0};};
struct Engine{std::vector<std::string>commands;void ClientCmd_Unrestricted(const char* c){commands.push_back(c);}}engineDouble;
Engine*engine=&engineDouble;
struct UI{bool visible=false;bool IsGameUIVisible(){return visible;}}uiDouble;
UI*enginevgui=&uiDouble;
struct ConVar{bool enabled=true;bool GetBool(){return enabled;}}touch_enable;
static int s_sliderFinger=-1;static float s_sliderSent=.6f;
enum{IE_FingerDown,IE_FingerUp,IE_FingerMotion,state_none,state_edit,SLIDER_NONE=-1};
static const float SLIDER_HIT_X2=.2f,SLIDER_Y1=.2f,SLIDER_Y2=.8f;
struct touch_event_t{int type=IE_FingerUp,fingerid=7;float x=.5f,y=.5f;};
int ActiveSlider(){return SLIDER_NONE;}void SliderMoved(int,float){}
struct CTouchControls{
 float forward=0,side=0,yaw=0,pitch=0,m_flPreviousYaw=0,m_flPreviousPitch=0,movecount=0;
 int look_finger=-1,move_finger=-1,wheel_finger=-1,resize_finger=-1,state=state_none;
 CTouchButton*move_button=nullptr;bool initialized=false,touchscreen=true;
 CUtlLinkedList<CTouchButton*>btns;int presses=0,motions=0,edits=0;
 bool HasTouchscreen(){return touchscreen;}void EditEvent(touch_event_t*){edits++;}void FingerMotion(touch_event_t*){motions++;}void FingerPress(touch_event_t*){presses++;}
 void ReleaseAllFingers();void GetTouchAccumulators(float*,float*,float*,float*);void ProcessEvent(touch_event_t*);
};
void hold(CTouchControls&t,CTouchButton&move,CTouchButton&attack,CTouchButton&jump){
 t.initialized=true;t.forward=1;t.side=-.5f;t.yaw=3;t.pitch=4;t.m_flPreviousYaw=2;t.m_flPreviousPitch=5;t.movecount=1;
 t.look_finger=2;t.move_finger=3;t.wheel_finger=4;t.resize_finger=5;t.move_button=&move;
 move.finger=3;attack.finger=6;jump.finger=7;s_sliderFinger=8;s_sliderSent=.6f;
}
void released(const CTouchControls&t){
 assert(t.forward==0&&t.side==0&&t.yaw==0&&t.pitch==0&&t.m_flPreviousYaw==0&&t.m_flPreviousPitch==0&&t.movecount==0);
 assert(t.look_finger==-1&&t.move_finger==-1&&t.wheel_finger==-1&&t.resize_finger==-1&&t.move_button==nullptr);
 assert(s_sliderFinger==-1&&s_sliderSent==-1);
 for(auto*b:t.btns)assert(b->finger==-1);
}
`;
const checks = String.raw`
int main(){
 CTouchControls t;CTouchButton move,attack,jump,unheld,toggle;
 std::strcpy(move.command,"_move");std::strcpy(attack.command,"+attack");std::strcpy(jump.command,"+jump");std::strcpy(unheld.command,"+use");std::strcpy(toggle.command,";+duck");
 t.btns={&move,&attack,&jump,&unheld,&toggle};
 // Safe before initialization, and release a shared slider only when it has a touch owner.
 t.forward=1;t.ReleaseAllFingers();assert(t.forward==0&&engineDouble.commands.empty()&&s_sliderSent==.6f);
 hold(t,move,attack,jump);toggle.finger=9;t.ReleaseAllFingers();released(t);
 assert((engineDouble.commands==std::vector<std::string>{"-attack","-jump"}));
 t.ReleaseAllFingers();assert(engineDouble.commands.size()==2);
 // Opening the main menu must release held commands even when no finger-up arrives.
 hold(t,move,attack,jump);uiDouble.visible=true;float side=9,forward=9,yaw=9,pitch=9;
 t.GetTouchAccumulators(&side,&forward,&yaw,&pitch);released(t);assert(side==0&&forward==0&&yaw==0&&pitch==0);
 assert(engineDouble.commands.size()==4);t.GetTouchAccumulators(&side,&forward,&yaw,&pitch);assert(engineDouble.commands.size()==4);
 // Disabling touch while moving also clears filtered look history and action buttons.
 uiDouble.visible=false;hold(t,move,attack,jump);touch_enable.enabled=false;
 t.GetTouchAccumulators(&side,&forward,&yaw,&pitch);released(t);assert(engineDouble.commands.size()==6);
 touch_event_t event;hold(t,move,attack,jump);t.ProcessEvent(&event);released(t);assert(t.presses==0&&engineDouble.commands.size()==8);
 touch_enable.enabled=true;uiDouble.visible=true;hold(t,move,attack,jump);t.ProcessEvent(&event);released(t);assert(t.presses==0&&engineDouble.commands.size()==10);
 uiDouble.visible=false;t.touchscreen=false;hold(t,move,attack,jump);t.ProcessEvent(&event);released(t);assert(t.presses==0&&engineDouble.commands.size()==12);
 t.touchscreen=true;t.ProcessEvent(&event);assert(t.presses==1);event.type=IE_FingerMotion;t.ProcessEvent(&event);assert(t.motions==1);
 t.state=state_edit;t.ProcessEvent(&event);assert(t.edits==1);
 // Normal accumulation preserves analog movement and consumes only relative look motion.
 t.forward=.7f;t.side=.2f;t.yaw=3;t.pitch=4;t.GetTouchAccumulators(&side,&forward,&yaw,&pitch);
 assert(forward==.7f&&side==.2f&&yaw==3&&pitch==4&&t.forward==.7f&&t.side==.2f&&t.yaw==0&&t.pitch==0);
 std::cout<<"PASS: touch release before init; move/look reset; attack/jump release once; menu/disable/no-touchscreen gates; slider ownership; normal event/analog flow\n";
}
`;
const cpp = path.join(output, 'touch_validation.cpp');
const exe = path.join(output, process.platform === 'win32' ? 'touch_validation.exe' : 'touch_validation');
fs.writeFileSync(cpp, preamble + ['ReleaseAllFingers', 'GetTouchAccumulators', 'ProcessEvent'].map(method).join('\n') + checks);
for (const [command, args] of [[process.env.CXX || 'g++', ['-std=c++11', '-Wall', '-Wextra', cpp, '-o', exe]], [exe, []]]) {
  const result = spawnSync(command, args, { stdio: 'inherit' });
  if (result.error) throw result.error;
  if (result.status !== 0) process.exit(result.status || 1);
}
