"Resource/UI/AdvancedVideo.res"
{
	"AdvancedVideo"
	{
		"ControlName"		"Frame"
		"fieldName"			"Video"
		"xpos"				"0"
		"ypos"				"0"
		"wide"				"11"
		"tall"				"5"
		"autoResize"		"0"
		"visible"			"1"
		"enabled"			"1"
		"tabPosition"		"0"
		"dialogstyle"		"1"
		"IgnoreButtonA"		"1"
	}

	"DrpAntialias"
	{
		"ControlName"		"BaseModHybridButton"
		"fieldName"			"DrpAntialias"
		"xpos"				"0"
		"ypos"				"25"
		"wide"				"350"
		"tall"				"20"
		"visible"			"1"
		"enabled"			"1"
		"IgnoreButtonA"		"1"
		"tabPosition"		"0"
		"navUp"				"DrpCrosshair"
		"navDown"			"DrpFiltering"
		"labelText"			"#L4D360UI_VideoOptions_Antialiasing"
		"style"				"DialogListButton"
		"list"
		{
			"_antialias0"	"_antialias0"
			"_antialias1"	"_antialias1"
			"_antialias2"	"_antialias2"
			"_antialias3"	"_antialias3"
			"_antialias4"	"_antialias4"
			"_antialias5"	"_antialias5"
			"_antialias6"	"_antialias6"
			"_antialias7"	"_antialias7"
			"_antialias8"	"_antialias8"
			"_antialias9"	"_antialias9"
		}
	}
	
	"DrpFiltering"
	{
		"ControlName"		"BaseModHybridButton"
		"fieldName"			"DrpFiltering"
		"xpos"				"0"
		"ypos"				"50"
		"wide"				"350"
		"tall"				"20"
		"visible"			"1"
		"enabled"			"1"
		"IgnoreButtonA"		"1"
		"tabPosition"		"0"
		"navUp"				"DrpAntialias"
		"navDown"			"DrpVSync"
		"labelText"			"#GameUI_Filtering_Mode"
		"style"				"DialogListButton"
		"list"
		{
			"#GameUI_Bilinear"			"#GameUI_Bilinear"
			"#GameUI_Trilinear"			"#GameUI_Trilinear"
			"#GameUI_Anisotropic2X"		"#GameUI_Anisotropic2X"
			"#GameUI_Anisotropic4X"		"#GameUI_Anisotropic4X"
			"#GameUI_Anisotropic8X"		"#GameUI_Anisotropic8X"
			"#GameUI_Anisotropic16X"	"#GameUI_Anisotropic16X"
		}
				
	}
	
	"DrpVSync"
	{
		"ControlName"		"BaseModHybridButton"
		"fieldName"			"DrpVSync"
		"xpos"				"0"
		"ypos"				"75"
		"wide"				"350"
		"tall"				"20"
		"visible"			"1"
		"enabled"			"1"
		"IgnoreButtonA"		"1"
		"tabPosition"		"0"
		"navUp"				"DrpFiltering"
		"navDown"			"DrpQueuedMode"
		"labelText"			"#GameUI_Wait_For_VSync"
		"style"				"DialogListButton"
		"list"
		{
			"#L4D360UI_Disabled"							"VSyncDisabled"
			"#L4D360UI_VideoOptions_VSync_DoubleBuffered"	"VSyncEnabled"
		}
	}
	
	"DrpQueuedMode"
	{
		"ControlName"		"BaseModHybridButton"
		"fieldName"			"DrpQueuedMode"
		"xpos"				"0"
		"ypos"				"100"
		"wide"				"350"
		"tall"				"20"
		"visible"			"1"
		"enabled"			"1"
		"IgnoreButtonA"		"1"
		"tabPosition"		"0"
		"navUp"				"DrpVSync"
		"navDown"			"DrpShaderDetail"
		"labelText"			"#L4D360UI_VideoOptions_Queued_Mode"
		"style"				"DialogListButton"
		"list"
		{
			"#L4D360UI_Disabled"	"QueuedModeDisabled"
			"#L4D360UI_Enabled"		"QueuedModeEnabled"
		}
	}
	
	"DrpShaderDetail"
	{
		"ControlName"		"BaseModHybridButton"
		"fieldName"			"DrpShaderDetail"
		"xpos"				"0"
		"ypos"				"125"
		"wide"				"350"
		"tall"				"20"
		"visible"			"1"
		"enabled"			"1"
		"IgnoreButtonA"		"1"
		"tabPosition"		"0"
		"navUp"				"DrpQueuedMode"
		"navDown"			"DrpCPUDetail"
		"labelText"			"#GameUI_Shader_Detail"
		"style"				"DialogListButton"
		"list"	
		{
			"#GameUI_Low"		"ShaderDetailLow"
			"#GameUI_Medium"	"ShaderDetailMedium"
			"#GameUI_High"		"ShaderDetailHigh"		
			"#FSTOP_VeryHigh"	"ShaderDetailVeryHigh"
		}
	}
	
	"DrpCPUDetail"
	{
		"ControlName"		"BaseModHybridButton"
		"fieldName"			"DrpCPUDetail"
		"xpos"				"0"
		"ypos"				"150"
		"wide"				"350"
		"tall"				"20"
		"visible"			"1"
		"enabled"			"1"
		"IgnoreButtonA"		"1"
		"tabPosition"		"0"
		"navUp"				"DrpShaderDetail"
		"navDown"			"DrpModelDetail"
		"labelText"			"#L4D360UI_VideoOptions_CPU_Detail"
		"style"				"DialogListButton"
		"list"	
		{
			"#GameUI_Low"		"CPUDetailLow"
			"#GameUI_Medium"	"CPUDetailMedium"
			"#GameUI_High"		"CPUDetailHigh"
		}
	}
	
	"DrpModelDetail"
	{
		"ControlName"		"BaseModHybridButton"
		"fieldName"			"DrpModelDetail"
		"xpos"				"0"
		"ypos"				"175"
		"wide"				"350"
		"tall"				"20"
		"visible"			"1"
		"enabled"			"1"
		"IgnoreButtonA"		"1"
		"tabPosition"		"0"
		"navUp"				"DrpCPUDetail"
		"navDown"			"DrpReflections"
		"labelText"			"#L4D360UI_VideoOptions_Model_Texture_Detail"
		"style"				"DialogListButton"
		"list"
		{
			"#GameUI_Low"		"ModelDetailLow"
			"#GameUI_Medium"	"ModelDetailMedium"
			"#GameUI_High"		"ModelDetailHigh"
			"#FSTOP_VeryHigh"	"ModelDetailVeryHigh"
		}		
	}
	

	"LblDescriptionTitle"
	{
		"ControlName"			"Label"
		"fieldName"				"LblDescriptionTitle"
		"xpos"					"360"
		"ypos"					"25"
		"wide"					"180"
		"tall"					"25"
		"visible"				"1"
		"enabled"				"1"
		"tabPosition"			"0"
		"Font"					"DialogMenuItem"
		"labelText"				"#L4D360UI_VideoOptions_Antialiasing"
		"textAlignment"			"west"
		"fgcolor_override"		"0 0 0 255"
		"bgcolor_override"		"0 0 0 0"
		"noshortcutsyntax"		"1"
	}

	"LblDescription"
	{
		"ControlName"			"Label"
		"fieldName"				"LblDescription"
		"xpos"					"360"
		"ypos"					"60"
		"wide"					"180"
		"tall"					"180"
		"visible"				"1"
		"enabled"				"1"
		"tabPosition"			"0"
		"Font"					"ConfirmationText"
		"labelText"				"#PORTAL2_VideoOptions_Antialiasing_Info"
		"textAlignment"			"north-west"
		"wrap"					"1"
		"fgcolor_override"		"40 40 40 255"
		"bgcolor_override"		"0 0 0 0"
		"noshortcutsyntax"		"1"
	}

 "DrpReflections"
 {
 "ControlName" "BaseModHybridButton"
 "fieldName" "DrpReflections"
 "xpos" "0"
 "ypos" "200"
 "wide" "350"
 "tall" "20"
 "visible" "1"
 "enabled" "1"
 "IgnoreButtonA" "1"
 "tabPosition" "0"
 "navUp" "DrpModelDetail"
 "navDown" "DrpCrosshair"
 "labelText" "Reflection quality"
 "style" "DialogListButton"
 "list"
 {
 "#GameUI_Low" "ReflectionLow"
 "#GameUI_Medium" "ReflectionMedium"
 "#GameUI_High" "ReflectionHigh"
 }
 }

 "DrpCrosshair"
 {
 "ControlName" "BaseModHybridButton"
 "fieldName" "DrpCrosshair"
 "xpos" "0"
 "ypos" "225"
 "wide" "350"
 "tall" "20"
 "visible" "1"
 "enabled" "1"
 "IgnoreButtonA" "1"
 "tabPosition" "0"
 "navUp" "DrpReflections"
 "navDown" "DrpAntialias"
 "labelText" "Crosshair"
 "style" "DialogListButton"
 "list"
 {
 "#L4D360UI_Disabled" "CrosshairOff"
 "#L4D360UI_Enabled" "CrosshairOn"
 }
 }
}
