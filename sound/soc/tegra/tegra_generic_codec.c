 /*
  * tegra_generic_codec.c  --  Generic codec interface for tegra
  *
  * Copyright  2011 Nvidia Graphics Pvt. Ltd.
  *
  * Author: Sumit Bhattacharya
  *             sumitb@nvidia.com
  *  http://www.nvidia.com
  *
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful, but WITHOUT
  * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  * more details.
  *
  * You should have received a copy of the GNU General Public License along
  * with this program; if not, write to the Free Software Foundation, Inc.,
  * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/soc.h>
#include <sound/initval.h>

#define DRV_NAME "tegra-generic-codec"

static struct snd_soc_dai_driver tegra_generic_codec_dai[] = {
	{
		.id = -1,
		.name = "tegra_generic_voice_codec",
		.playback = {
			.stream_name    = "Playback",
			.channels_min   = 1,
			.channels_max   = 1,
			.rates          = SNDRV_PCM_RATE_8000,
			.formats        = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.capture = {
			.stream_name    = "Capture",
			.channels_min   = 1,
			.channels_max   = 1,
			.rates          = SNDRV_PCM_RATE_8000,
			.formats        = SNDRV_PCM_FMTBIT_S16_LE,
		},
	}
};

static struct snd_soc_codec_driver tegra_generic_codec;

static __devinit int generic_codec_probe(struct platform_device *pdev)
{
	int ret = 0;

	ret = snd_soc_register_codec(&pdev->dev,&tegra_generic_codec,
			tegra_generic_codec_dai,ARRAY_SIZE(tegra_generic_codec_dai));
	if (ret != 0) {
		dev_dbg(&pdev->dev,"codec: failed to register tegra_generic_codec\n");
		return ret;
	}

	return ret;
}

static int __devexit generic_codec_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static struct platform_driver tegra_generic_codec_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
	},
	.probe = generic_codec_probe,
	.remove = __devexit_p(generic_codec_remove),
};

static int __init tegra_generic_codec_init(void)
{
	return platform_driver_register(&tegra_generic_codec_driver);
}
module_init(tegra_generic_codec_init);

static void __exit tegra_generic_codec_exit(void)
{
	platform_driver_unregister(&tegra_generic_codec_driver);
}
module_exit(tegra_generic_codec_exit);

/* Module information */
MODULE_AUTHOR("MVIDIA/Eduardo José Tagle <ejtagle@tutopia.com>");
MODULE_DESCRIPTION("Tegra ALSA Generic Codec Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);

