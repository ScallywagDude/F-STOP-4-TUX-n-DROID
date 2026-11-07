#ifndef ENV_DOF_CONTROLLER_H
#define ENV_DOF_CONTROLLER_H
// The mobile renderer has no depth-of-field pass, so the controller only stores the settings.
struct DOFControlSettings_t
{
	bool	bIsEnabled;
	float	flNearBlurDepth, flNearFocusDistance, flNearBlurRadius;
	float	flFarBlurDepth, flFarFocusDistance, flFarBlurRadius;
};
class CEnvDOFController : public CPointEntity
{
	DECLARE_CLASS( CEnvDOFController, CPointEntity );
public:
	void SetControllerState( DOFControlSettings_t &settings ) { m_Settings = settings; }
private:
	DOFControlSettings_t m_Settings;
};
#endif
