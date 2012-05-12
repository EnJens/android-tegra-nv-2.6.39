#ifndef _INCLUDE_SOUND_ALC5623_H
#define _INCLUDE_SOUND_ALC5623_H
struct alc5623_platform_data {
	unsigned int	avdd_mv;		/* Analog vdd in millivolts */

	unsigned int	mic1bias_mv;	/* MIC1 bias voltage */
	unsigned int	mic2bias_mv;	/* MIC2	bias voltage */
	unsigned int	mic1boost_db;	/* MIC1 gain boost */
	unsigned int	mic2boost_db;	/* MIC1 gain boost */

	bool		default_is_mic2;/* Default MIC used as input will be MIC2. Otherwise MIC1 is used */
	/* configure :                              */
	/* Lineout/Speaker Amps Vmid ratio control  */
	/* enable/disable adc/dac high pass filters */
	unsigned int add_ctrl;
	/* configure :                              */
	/* output to enable when jack is low        */
	/* output to enable when jack is high       */
	/* jack detect (gpio/nc/jack detect [12]    */
	unsigned int jack_det_ctrl;
};
#endif

