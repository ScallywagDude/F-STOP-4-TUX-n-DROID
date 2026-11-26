// The tile flip transition belongs to the animated Portal 2 background, which this game does not use.
#ifndef TRANSITIONPANEL_SHIM_H
#define TRANSITIONPANEL_SHIM_H
namespace BaseModUI
{
class CBaseModTransitionPanel
{
public:
	void SetExpectedDirection( bool, int ) {}
	void MarkTilesInRect( int, int, int, int, int = 0, bool = false ) {}
	void MarkTile( int, int, int ) {}
	void SetInitialState() {}
	void TurnOffTransitions() {}
	void PreventTransitions( bool ) {}
};
}
#endif
