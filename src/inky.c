#include <inky-api.h>

/*
**********************************************************************
*********************** Library Definitions **************************
**********************************************************************
*/

/* Commands recognized by peripheral */
typedef enum {
	SOFT_RESET		= 0x12, /* Soft Reset */
	ANALOG_BLOCK_CONTROL	= 0x74, /* Set Analog Block Control */
	DIGITAL_BLOCK_CONTROL	= 0x7e, /* Set Digital Block Control */
	GATE_SETTING		= 0x01, /* Gate setting */
	GATE_DRIVING_VOLTAGE	= 0x03, /* Gate Driving Voltage */
	SOURCE_DRIVING_VOLTAGE	= 0x04, /* Source Driving Voltage */
	DUMMY_LINE_PERIOD	= 0x3a, /* Dummy line period */
	GATE_LINE_WIDTH		= 0x3b, /* Gate line width */
	DATA_ENTRY_MODE		= 0x11, /* Data entry mode setting 0x03 = X/Y increment */
	VCOM_REGISTER		= 0x2c, /* VCOM Register, 0x3c = -1.5v? */
	GS_TRANSITION_DEFINE	= 0x3c, /* GS Transition Define A + VSS + LUT0 */
	SET_LUTS		= 0x32, /* Set LUTs */
	RAM_X_RANGE		= 0x44, /* Set RAM X Start/End */
	RAM_Y_RANGE		= 0x45, /* Set RAM Y Start/End */
	RAM_X_PTR_START		= 0x4e, /* Set RAM X Pointer Start */
	RAM_Y_PTR_START		= 0x4f, /* Set RAM Y Pointer Start */
	UPDATE_SEQUENCE		= 0x22, /* Display Update Sequence */
	TRIGGER_UPDATE		= 0x20, /* Trigger Display Update */
	ENTER_DEEP_SLEEP	= 0x10 /* Enter Deep Sleep */
} dcommand;

