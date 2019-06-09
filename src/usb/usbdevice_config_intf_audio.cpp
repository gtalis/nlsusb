/*
    Copyright (C) 1999-2001, 2003 Thomas Sailer (t.sailer@alumni.ethz.ch)
    Copyright (C) 2003-2005 David Brownell
    Copyright (C) 2018  Gilles Talis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "usbdevice.h"
#include "names.h"
#include "usbmisc.h"

#include <stdio.h>
#include <string.h>

using namespace std;


/* ---------------------------------------------------------------------- */

/*
 * Audio Class descriptor dump
 */

static void dump_audio_subtype(libusb_device_handle *dev,
                               const char *name,
                               const struct desc * const desc[3],
                               const unsigned char *buf,
                               int protocol,
                               unsigned int indent)
{
	static const char * const strings[] = { "UAC1", "UAC2", "UAC3" };
	unsigned int idx = 0;

	switch (protocol) {
	case USB_AUDIO_CLASS_2: idx = 1; break;
	case USB_AUDIO_CLASS_3: idx = 2; break;
	}

	printf("(%s)\n", name);

	if (desc[idx] == NULL) {
		printf("%*sWarning: %s descriptors are illegal for %s\n",
		       indent * 2, "", name, strings[idx]);
		return;
	}

	/* Skip the first three bytes; those common fields have already
	 * been dumped. */
	desc_dump(dev, desc[idx], buf + 3, buf[0] - 3, indent);
}

/* USB Audio Class subtypes */
enum uac_interface_subtype {
	UAC_INTERFACE_SUBTYPE_AC_DESCRIPTOR_UNDEFINED = 0x00,
	UAC_INTERFACE_SUBTYPE_HEADER                  = 0x01,
	UAC_INTERFACE_SUBTYPE_INPUT_TERMINAL          = 0x02,
	UAC_INTERFACE_SUBTYPE_OUTPUT_TERMINAL         = 0x03,
	UAC_INTERFACE_SUBTYPE_EXTENDED_TERMINAL       = 0x04,
	UAC_INTERFACE_SUBTYPE_MIXER_UNIT              = 0x05,
	UAC_INTERFACE_SUBTYPE_SELECTOR_UNIT           = 0x06,
	UAC_INTERFACE_SUBTYPE_FEATURE_UNIT            = 0x07,
	UAC_INTERFACE_SUBTYPE_EFFECT_UNIT             = 0x08,
	UAC_INTERFACE_SUBTYPE_PROCESSING_UNIT         = 0x09,
	UAC_INTERFACE_SUBTYPE_EXTENSION_UNIT          = 0x0a,
	UAC_INTERFACE_SUBTYPE_CLOCK_SOURCE            = 0x0b,
	UAC_INTERFACE_SUBTYPE_CLOCK_SELECTOR          = 0x0c,
	UAC_INTERFACE_SUBTYPE_CLOCK_MULTIPLIER        = 0x0d,
	UAC_INTERFACE_SUBTYPE_SAMPLE_RATE_CONVERTER   = 0x0e,
	UAC_INTERFACE_SUBTYPE_CONNECTORS              = 0x0f,
	UAC_INTERFACE_SUBTYPE_POWER_DOMAIN            = 0x10,
};

/*
 * UAC1, UAC2, and UAC3 define bDescriptorSubtype differently for the
 * AudioControl interface, so we need to do some ugly remapping:
 *
 * val  | UAC1            | UAC2                  | UAC3
 * -----|-----------------|-----------------------|---------------------
 * 0x00 | AC UNDEFINED    | AC UNDEFINED          | AC UNDEFINED
 * 0x01 | HEADER          | HEADER                | HEADER
 * 0x02 | INPUT_TERMINAL  | INPUT_TERMINAL        | INPUT_TERMINAL
 * 0x03 | OUTPUT_TERMINAL | OUTPUT_TERMINAL       | OUTPUT_TERMINAL
 * 0x04 | MIXER_UNIT      | MIXER_UNIT            | EXTENDED_TERMINAL
 * 0x05 | SELECTOR_UNIT   | SELECTOR_UNIT         | MIXER_UNIT
 * 0x06 | FEATURE_UNIT    | FEATURE_UNIT          | SELECTOR_UNIT
 * 0x07 | PROCESSING_UNIT | EFFECT_UNIT           | FEATURE_UNIT
 * 0x08 | EXTENSION_UNIT  | PROCESSING_UNIT       | EFFECT_UNIT
 * 0x09 | -               | EXTENSION_UNIT        | PROCESSING_UNIT
 * 0x0a | -               | CLOCK_SOURCE          | EXTENSION_UNIT
 * 0x0b | -               | CLOCK_SELECTOR        | CLOCK_SOURCE
 * 0x0c | -               | CLOCK_MULTIPLIER      | CLOCK_SELECTOR
 * 0x0d | -               | SAMPLE_RATE_CONVERTER | CLOCK_MULTIPLIER
 * 0x0e | -               | -                     | SAMPLE_RATE_CONVERTER
 * 0x0f | -               | -                     | CONNECTORS
 * 0x10 | -               | -                     | POWER_DOMAIN
 */
static enum uac_interface_subtype get_uac_interface_subtype(unsigned char c, int protocol)
{
	switch (protocol) {
	case USB_AUDIO_CLASS_1:
		switch(c) {
		case 0x04: return UAC_INTERFACE_SUBTYPE_MIXER_UNIT;
		case 0x05: return UAC_INTERFACE_SUBTYPE_SELECTOR_UNIT;
		case 0x06: return UAC_INTERFACE_SUBTYPE_FEATURE_UNIT;
		case 0x07: return UAC_INTERFACE_SUBTYPE_PROCESSING_UNIT;
		case 0x08: return UAC_INTERFACE_SUBTYPE_EXTENSION_UNIT;
		}
		break;
	case USB_AUDIO_CLASS_2:
		switch(c) {
		case 0x04: return UAC_INTERFACE_SUBTYPE_MIXER_UNIT;
		case 0x05: return UAC_INTERFACE_SUBTYPE_SELECTOR_UNIT;
		case 0x06: return UAC_INTERFACE_SUBTYPE_FEATURE_UNIT;
		case 0x07: return UAC_INTERFACE_SUBTYPE_EFFECT_UNIT;
		case 0x08: return UAC_INTERFACE_SUBTYPE_PROCESSING_UNIT;
		case 0x09: return UAC_INTERFACE_SUBTYPE_EXTENSION_UNIT;
		case 0x0a: return UAC_INTERFACE_SUBTYPE_CLOCK_SOURCE;
		case 0x0b: return UAC_INTERFACE_SUBTYPE_CLOCK_SELECTOR;
		case 0x0c: return UAC_INTERFACE_SUBTYPE_CLOCK_MULTIPLIER;
		case 0x0d: return UAC_INTERFACE_SUBTYPE_SAMPLE_RATE_CONVERTER;
		}
		break;
	case USB_AUDIO_CLASS_3:
		/* No mapping required. */
		break;
	default:
		/* Unknown protocol */
		break;
	}

	/* If the protocol was unknown, or the value was not known to require
	 * mapping, just return it unchanged. */
	return c;
}

static void dump_audiocontrol_interface(libusb_device_handle *dev, const unsigned char *buf, int protocol)
{
	enum uac_interface_subtype subtype;

	if (buf[1] != USB_DT_CS_INTERFACE)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 3)
		printf("      Warning: Descriptor too short\n");
	printf("      AudioControl Interface Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bDescriptorSubtype  %5u ",
	       buf[0], buf[1], buf[2]);

	subtype = get_uac_interface_subtype(buf[2], protocol);

	switch (subtype) {
	case UAC_INTERFACE_SUBTYPE_HEADER:
		dump_audio_subtype(dev, "HEADER", desc_audio_ac_header, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_INPUT_TERMINAL:
		dump_audio_subtype(dev, "INPUT_TERMINAL", desc_audio_ac_input_terminal, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_OUTPUT_TERMINAL:
		dump_audio_subtype(dev, "OUTPUT_TERMINAL", desc_audio_ac_output_terminal, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_MIXER_UNIT:
		dump_audio_subtype(dev, "MIXER_UNIT", desc_audio_ac_mixer_unit, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_SELECTOR_UNIT:
		dump_audio_subtype(dev, "SELECTOR_UNIT", desc_audio_ac_selector_unit, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_FEATURE_UNIT:
		dump_audio_subtype(dev, "FEATURE_UNIT", desc_audio_ac_feature_unit, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_PROCESSING_UNIT:
		dump_audio_subtype(dev, "PROCESSING_UNIT", desc_audio_ac_processing_unit, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_EXTENSION_UNIT:
		dump_audio_subtype(dev, "EXTENSION_UNIT", desc_audio_ac_extension_unit, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_CLOCK_SOURCE:
		dump_audio_subtype(dev, "CLOCK_SOURCE", desc_audio_ac_clock_source, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_CLOCK_SELECTOR:
		dump_audio_subtype(dev, "CLOCK_SELECTOR", desc_audio_ac_clock_selector, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_CLOCK_MULTIPLIER:
		dump_audio_subtype(dev, "CLOCK_MULTIPLIER", desc_audio_ac_clock_multiplier, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_SAMPLE_RATE_CONVERTER:
		dump_audio_subtype(dev, "SAMPLING_RATE_CONVERTER", desc_audio_ac_clock_multiplier, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_EFFECT_UNIT:
		dump_audio_subtype(dev, "EFFECT_UNIT", desc_audio_ac_effect_unit, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_POWER_DOMAIN:
		dump_audio_subtype(dev, "POWER_DOMAIN", desc_audio_ac_power_domain, buf, protocol, 4);
		break;

	default:
		printf("(unknown)\n"
		       "        Invalid desc subtype:");
		dump_bytes(buf+3, buf[0]-3);
		break;
	}
}


static void dump_audiostreaming_interface(libusb_device_handle *dev, const unsigned char *buf, int protocol)
{
	static const char * const fmtItag[] = {
		"TYPE_I_UNDEFINED", "PCM", "PCM8", "IEEE_FLOAT", "ALAW", "MULAW" };
	static const char * const fmtIItag[] = { "TYPE_II_UNDEFINED", "MPEG", "AC-3" };
	static const char * const fmtIIItag[] = {
		"TYPE_III_UNDEFINED", "IEC1937_AC-3", "IEC1937_MPEG-1_Layer1",
		"IEC1937_MPEG-Layer2/3/NOEXT", "IEC1937_MPEG-2_EXT",
		"IEC1937_MPEG-2_Layer1_LS", "IEC1937_MPEG-2_Layer2/3_LS" };
	unsigned int i, j, fmttag;
	const char *fmtptr = "undefined";
	char *name = NULL;

	if (buf[1] != USB_DT_CS_INTERFACE)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 3)
		printf("      Warning: Descriptor too short\n");
	printf("      AudioStreaming Interface Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bDescriptorSubtype  %5u ",
	       buf[0], buf[1], buf[2]);
	switch (buf[2]) {
	case 0x01: /* AS_GENERAL */
		dump_audio_subtype(dev, "AS_GENERAL", desc_audio_as_interface, buf, protocol, 4);
		break;

	case 0x02: /* FORMAT_TYPE */
		printf("(FORMAT_TYPE)\n");
		switch (protocol) {
		case USB_AUDIO_CLASS_1:
			if (buf[0] < 8)
				printf("      Warning: Descriptor too short\n");
			printf("        bFormatType         %5u ", buf[3]);
			switch (buf[3]) {
			case 0x01: /* FORMAT_TYPE_I */
				printf("(FORMAT_TYPE_I)\n");
				j = buf[7] ? (buf[7]*3+8) : 14;
				if (buf[0] < j)
					printf("      Warning: Descriptor too short\n");
				printf("        bNrChannels         %5u\n"
				       "        bSubframeSize       %5u\n"
				       "        bBitResolution      %5u\n"
				       "        bSamFreqType        %5u %s\n",
				       buf[4], buf[5], buf[6], buf[7], buf[7] ? "Discrete" : "Continuous");
				if (!buf[7])
					printf("        tLowerSamFreq     %7u\n"
					       "        tUpperSamFreq     %7u\n",
					       buf[8] | (buf[9] << 8) | (buf[10] << 16), buf[11] | (buf[12] << 8) | (buf[13] << 16));
				else
					for (i = 0; i < buf[7]; i++)
						printf("        tSamFreq[%2u]      %7u\n", i,
						       buf[8+3*i] | (buf[9+3*i] << 8) | (buf[10+3*i] << 16));
				dump_junk(buf, "        ", j);
				break;

			case 0x02: /* FORMAT_TYPE_II */
				printf("(FORMAT_TYPE_II)\n");
				j = buf[8] ? (buf[7]*3+9) : 15;
				if (buf[0] < j)
					printf("      Warning: Descriptor too short\n");
				printf("        wMaxBitRate         %5u\n"
				       "        wSamplesPerFrame    %5u\n"
				       "        bSamFreqType        %5u %s\n",
				       buf[4] | (buf[5] << 8), buf[6] | (buf[7] << 8), buf[8], buf[8] ? "Discrete" : "Continuous");
				if (!buf[8])
					printf("        tLowerSamFreq     %7u\n"
					       "        tUpperSamFreq     %7u\n",
					       buf[9] | (buf[10] << 8) | (buf[11] << 16), buf[12] | (buf[13] << 8) | (buf[14] << 16));
				else
					for (i = 0; i < buf[8]; i++)
						printf("        tSamFreq[%2u]      %7u\n", i,
						       buf[9+3*i] | (buf[10+3*i] << 8) | (buf[11+3*i] << 16));
				dump_junk(buf, "        ", j);
				break;

			case 0x03: /* FORMAT_TYPE_III */
				printf("(FORMAT_TYPE_III)\n");
				j = buf[7] ? (buf[7]*3+8) : 14;
				if (buf[0] < j)
					printf("      Warning: Descriptor too short\n");
				printf("        bNrChannels         %5u\n"
				       "        bSubframeSize       %5u\n"
				       "        bBitResolution      %5u\n"
				       "        bSamFreqType        %5u %s\n",
				       buf[4], buf[5], buf[6], buf[7], buf[7] ? "Discrete" : "Continuous");
				if (!buf[7])
					printf("        tLowerSamFreq     %7u\n"
					       "        tUpperSamFreq     %7u\n",
					       buf[8] | (buf[9] << 8) | (buf[10] << 16), buf[11] | (buf[12] << 8) | (buf[13] << 16));
				else
					for (i = 0; i < buf[7]; i++)
						printf("        tSamFreq[%2u]      %7u\n", i,
						       buf[8+3*i] | (buf[9+3*i] << 8) | (buf[10+3*i] << 16));
				dump_junk(buf, "        ", j);
				break;

			default:
				printf("(unknown)\n"
				       "        Invalid desc format type:");
				dump_bytes(buf+4, buf[0]-4);
			}

			break;

		case USB_AUDIO_CLASS_2:
			printf("        bFormatType         %5u ", buf[3]);
			switch (buf[3]) {
			case 0x01: /* FORMAT_TYPE_I */
				printf("(FORMAT_TYPE_I)\n");
				if (buf[0] < 6)
					printf("      Warning: Descriptor too short\n");
				printf("        bSubslotSize        %5u\n"
				       "        bBitResolution      %5u\n",
				       buf[4], buf[5]);
				dump_junk(buf, "        ", 6);
				break;

			case 0x02: /* FORMAT_TYPE_II */
				printf("(FORMAT_TYPE_II)\n");
				if (buf[0] < 8)
					printf("      Warning: Descriptor too short\n");
				printf("        wMaxBitRate         %5u\n"
				       "        wSlotsPerFrame      %5u\n",
				       buf[4] | (buf[5] << 8),
				       buf[6] | (buf[7] << 8));
				dump_junk(buf, "        ", 8);
				break;

			case 0x03: /* FORMAT_TYPE_III */
				printf("(FORMAT_TYPE_III)\n");
				if (buf[0] < 6)
					printf("      Warning: Descriptor too short\n");
				printf("        bSubslotSize        %5u\n"
				       "        bBitResolution      %5u\n",
				       buf[4], buf[5]);
				dump_junk(buf, "        ", 6);
				break;

			case 0x04: /* FORMAT_TYPE_IV */
				printf("(FORMAT_TYPE_IV)\n");
				if (buf[0] < 4)
					printf("      Warning: Descriptor too short\n");
				printf("        bFormatType         %5u\n", buf[3]);
				dump_junk(buf, "        ", 4);
				break;

			default:
				printf("(unknown)\n"
				       "        Invalid desc format type:");
				dump_bytes(buf+4, buf[0]-4);
			}

			break;
		} /* switch (protocol) */

		break;

	case 0x03: /* FORMAT_SPECIFIC */
		printf("(FORMAT_SPECIFIC)\n");
		if (buf[0] < 5)
			printf("      Warning: Descriptor too short\n");
		fmttag = buf[3] | (buf[4] << 8);
		if (fmttag <= 5)
			fmtptr = fmtItag[fmttag];
		else if (fmttag >= 0x1000 && fmttag <= 0x1002)
			fmtptr = fmtIItag[fmttag & 0xfff];
		else if (fmttag >= 0x2000 && fmttag <= 0x2006)
			fmtptr = fmtIIItag[fmttag & 0xfff];
		printf("        wFormatTag          %5u %s\n", fmttag, fmtptr);
		switch (fmttag) {
		case 0x1001: /* MPEG */
			if (buf[0] < 8)
				printf("      Warning: Descriptor too short\n");
			printf("        bmMPEGCapabilities 0x%04x\n",
			       buf[5] | (buf[6] << 8));
			if (buf[5] & 0x01)
				printf("          Layer I\n");
			if (buf[5] & 0x02)
				printf("          Layer II\n");
			if (buf[5] & 0x04)
				printf("          Layer III\n");
			if (buf[5] & 0x08)
				printf("          MPEG-1 only\n");
			if (buf[5] & 0x10)
				printf("          MPEG-1 dual-channel\n");
			if (buf[5] & 0x20)
				printf("          MPEG-2 second stereo\n");
			if (buf[5] & 0x40)
				printf("          MPEG-2 7.1 channel augmentation\n");
			if (buf[5] & 0x80)
				printf("          Adaptive multi-channel prediction\n");
			printf("          MPEG-2 multilingual support: ");
			switch (buf[6] & 3) {
			case 0:
				printf("Not supported\n");
				break;

			case 1:
				printf("Supported at Fs\n");
				break;

			case 2:
				printf("Reserved\n");
				break;

			default:
				printf("Supported at Fs and 1/2Fs\n");
				break;
			}
			printf("        bmMPEGFeatures       0x%02x\n", buf[7]);
			printf("          Internal Dynamic Range Control: ");
			switch ((buf[7] >> 4) & 3) {
			case 0:
				printf("not supported\n");
				break;

			case 1:
				printf("supported but not scalable\n");
				break;

			case 2:
				printf("scalable, common boost and cut scaling value\n");
				break;

			default:
				printf("scalable, separate boost and cut scaling value\n");
				break;
			}
			dump_junk(buf, "        ", 8);
			break;

		case 0x1002: /* AC-3 */
			if (buf[0] < 10)
				printf("      Warning: Descriptor too short\n");
			printf("        bmBSID         0x%08x\n"
			       "        bmAC3Features        0x%02x\n",
			       buf[5] | (buf[6] << 8) | (buf[7] << 16) | (buf[8] << 24), buf[9]);
			if (buf[9] & 0x01)
				printf("          RF mode\n");
			if (buf[9] & 0x02)
				printf("          Line mode\n");
			if (buf[9] & 0x04)
				printf("          Custom0 mode\n");
			if (buf[9] & 0x08)
				printf("          Custom1 mode\n");
			printf("          Internal Dynamic Range Control: ");
			switch ((buf[9] >> 4) & 3) {
			case 0:
				printf("not supported\n");
				break;

			case 1:
				printf("supported but not scalable\n");
				break;

			case 2:
				printf("scalable, common boost and cut scaling value\n");
				break;

			default:
				printf("scalable, separate boost and cut scaling value\n");
				break;
			}
			dump_junk(buf, "        ", 8);
			break;

		default:
			printf("(unknown)\n"
			       "        Invalid desc format type:");
			dump_bytes(buf+4, buf[0]-4);
		}
		break;

	default:
		printf("        Invalid desc subtype:");
		dump_bytes(buf+3, buf[0]-3);
		break;
	}

	free(name);
}

static void dump_audiostreaming_endpoint(libusb_device_handle *dev, const unsigned char *buf, int protocol)
{
	static const char * const subtype[] = { "invalid", "EP_GENERAL" };

	if (buf[1] != USB_DT_CS_ENDPOINT)
		printf("      Warning: Invalid descriptor\n");

	printf("        AudioStreaming Endpoint Descriptor:\n"
	       "          bLength             %5u\n"
	       "          bDescriptorType     %5u\n"
	       "          bDescriptorSubtype  %5u ",
	       buf[0], buf[1], buf[2]);

	dump_audio_subtype(dev, subtype[buf[2] == 1],
			desc_audio_as_isochronous_audio_data_endpoint, buf, protocol, 5);
}

static void dump_midistreaming_interface(libusb_device_handle *dev, const unsigned char *buf)
{
	static const char * const jacktypes[] = {"Undefined", "Embedded", "External"};
	char *jackstr = NULL;
	unsigned int j, tlength, capssize;
	unsigned long caps;

	if (buf[1] != USB_DT_CS_INTERFACE)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 3)
		printf("      Warning: Descriptor too short\n");
	printf("      MIDIStreaming Interface Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bDescriptorSubtype  %5u ",
	       buf[0], buf[1], buf[2]);
	switch (buf[2]) {
	case 0x01:
		printf("(HEADER)\n");
		if (buf[0] < 7)
			printf("      Warning: Descriptor too short\n");
		tlength = buf[5] | (buf[6] << 8);
		printf("        bcdADC              %2x.%02x\n"
		       "        wTotalLength       0x%04x\n",
		       buf[4], buf[3], tlength);
		dump_junk(buf, "        ", 7);
		break;

	case 0x02:
		printf("(MIDI_IN_JACK)\n");
		if (buf[0] < 6)
			printf("      Warning: Descriptor too short\n");
		jackstr = get_dev_string(dev, buf[5]);
		printf("        bJackType           %5u %s\n"
		       "        bJackID             %5u\n"
		       "        iJack               %5u %s\n",
		       buf[3], buf[3] < 3 ? jacktypes[buf[3]] : "Invalid",
		       buf[4], buf[5], jackstr);
		dump_junk(buf, "        ", 6);
		break;

	case 0x03:
		printf("(MIDI_OUT_JACK)\n");
		if (buf[0] < 9)
			printf("      Warning: Descriptor too short\n");
		printf("        bJackType           %5u %s\n"
		       "        bJackID             %5u\n"
		       "        bNrInputPins        %5u\n",
		       buf[3], buf[3] < 3 ? jacktypes[buf[3]] : "Invalid",
		       buf[4], buf[5]);
		for (j = 0; j < buf[5]; j++) {
			printf("        baSourceID(%2u)      %5u\n"
			       "        BaSourcePin(%2u)     %5u\n",
			       j, buf[2*j+6], j, buf[2*j+7]);
		}
		j = 6+buf[5]*2; /* midi10.pdf says, incorrectly: 5+2*p */
		jackstr = get_dev_string(dev, buf[j]);
		printf("        iJack               %5u %s\n",
		       buf[j], jackstr);
		dump_junk(buf, "        ", j+1);
		break;

	case 0x04:
		printf("(ELEMENT)\n");
		if (buf[0] < 12)
			printf("      Warning: Descriptor too short\n");
		printf("        bElementID          %5u\n"
		       "        bNrInputPins        %5u\n",
		       buf[3], buf[4]);
		for (j = 0; j < buf[4]; j++) {
			printf("        baSourceID(%2u)      %5u\n"
			       "        BaSourcePin(%2u)     %5u\n",
			       j, buf[2*j+5], j, buf[2*j+6]);
		}
		j = 5+buf[4]*2;
		printf("        bNrOutputPins       %5u\n"
		       "        bInTerminalLink     %5u\n"
		       "        bOutTerminalLink    %5u\n"
		       "        bElCapsSize         %5u\n",
		       buf[j], buf[j+1], buf[j+2], buf[j+3]);
		capssize = buf[j+3];
		caps = 0;
		for (j = 0; j < capssize; j++)
			caps |= (buf[j+9+buf[4]*2] << (8*j));
		printf("        bmElementCaps  0x%08lx\n", caps);
		if (caps & 0x01)
			printf("          Undefined\n");
		if (caps & 0x02)
			printf("          MIDI Clock\n");
		if (caps & 0x04)
			printf("          MTC (MIDI Time Code)\n");
		if (caps & 0x08)
			printf("          MMC (MIDI Machine Control)\n");
		if (caps & 0x10)
			printf("          GM1 (General MIDI v.1)\n");
		if (caps & 0x20)
			printf("          GM2 (General MIDI v.2)\n");
		if (caps & 0x40)
			printf("          GS MIDI Extension\n");
		if (caps & 0x80)
			printf("          XG MIDI Extension\n");
		if (caps & 0x100)
			printf("          EFX\n");
		if (caps & 0x200)
			printf("          MIDI Patch Bay\n");
		if (caps & 0x400)
			printf("          DLS1 (Downloadable Sounds Level 1)\n");
		if (caps & 0x800)
			printf("          DLS2 (Downloadable Sounds Level 2)\n");
		j = 9+2*buf[4]+capssize;
		jackstr = get_dev_string(dev, buf[j]);
		printf("        iElement            %5u %s\n", buf[j], jackstr);
		dump_junk(buf, "        ", j+1);
		break;

	default:
		printf("\n        Invalid desc subtype: ");
		dump_bytes(buf+3, buf[0]-3);
		break;
	}

	free(jackstr);
}

static void dump_midistreaming_endpoint(const unsigned char *buf)
{
	unsigned int j;

	if (buf[1] != USB_DT_CS_ENDPOINT)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 5)
		printf("      Warning: Descriptor too short\n");
	printf("        MIDIStreaming Endpoint Descriptor:\n"
	       "          bLength             %5u\n"
	       "          bDescriptorType     %5u\n"
	       "          bDescriptorSubtype  %5u (%s)\n"
	       "          bNumEmbMIDIJack     %5u\n",
	       buf[0], buf[1], buf[2], buf[2] == 1 ? "GENERAL" : "Invalid", buf[3]);
	for (j = 0; j < buf[3]; j++)
		printf("          baAssocJackID(%2u)   %5u\n", j, buf[4+j]);
	dump_junk(buf, "          ", 4+buf[3]);
}