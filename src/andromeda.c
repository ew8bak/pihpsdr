#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "receiver.h"
#include "toolbar.h"
#include "band_menu.h"
#include "sliders.h"
#include "rigctl.h"
#include "radio.h"
#include "channel.h"
#include "filter.h"
#include "mode.h"
#include "filter.h"
#include "band.h"
#include "bandstack.h"
#include "filter_menu.h"
#include "vfo.h"
#include "sliders.h"
#include "transmitter.h"
#include "agc.h"
#include <wdsp.h>
#include "store.h"
#include "ext.h"
#include "rigctl_menu.h"
#include "noise_menu.h"
#include "new_protocol.h"
#include <math.h>
#include "actions.h"
#include "andromeda.h"

gboolean implemented;

void parseAndromedaCommand(char *cmd)
{
    switch (cmd[3])
    {
    case 'U': // ZZZU, VFO Encoder Up
        ZZZU(cmd);
        break;
    case 'D': // ZZZD, VFO Encoder Down
        ZZZD(cmd);
        break;
    case 'P': // ZZZP, Button
        ZZZP(cmd);
        break;
    case 'E': // ZZZP, Encoder
        ZZZE(cmd);
        break;
    default:
        implemented = FALSE;
        break;
    }
}

void ZZZU(char *cmd)
{
    vfo_step(1);
}

void ZZZD(char *cmd)
{
    vfo_step(-1);
}

void ZZZP(char *cmd)
{
    if (cmd[7] == ';')
    {
        gboolean State = FALSE;
        gboolean LongPress = FALSE;
        char idButton[3];
        strncpy(idButton, cmd + 4, 3);
        int Button = atoi(idButton);

        if ((Button % 10) == 1)
            State = TRUE;
        else if ((Button % 10) == 2)
            LongPress = TRUE;
        Button /= 10;
        printf("Button:%d", Button - 1);
        HandleFrontPanelButtonPress(Button - 1, State, LongPress);
    }
}

void ZZZE(char *cmd)
{
    if (cmd[7] == ';')
    {
        char idEncoder[3];
        strncpy(idEncoder, cmd + 4, 3);
        int Encoder = atoi(idEncoder);
        int Step = Encoder % 10;
        Encoder /= 10;
        if ((Encoder >= 1) && (Encoder <= 20))
        {
            HandleFrontPanelEncoderStep(Encoder - 1, Step);
            printf("Encoder:%d, Step:%d", Encoder - 1, Step);
        }
        else if ((Encoder >= 51) && (Encoder <= 70))
        {
            HandleFrontPanelEncoderStep(Encoder - 51, -Step);
            printf("Encoder:%d, Step:%d", Encoder - 51, -Step);
        }
    }
}

void HandleFrontPanelButtonPress(int Button, gboolean State, gboolean LongPress)
{
    if ((Button >= 29) && (Button <= 40))
    {
        BandButton(Button);
    }
    if (Button == 41)
    { // RIT XIT
        RitXitButton(State, LongPress);
    }
    if ((Button >= 20) && (Button <= 27))
    {
        FuncButton(Button, State);
    }
}

void HandleFrontPanelEncoderStep(int Encoder, int Step)
{
    if (Encoder == 0) // AF
        af_encoder_change(Step, 1);
    if (Encoder == 1) // AGC
        rf_encoder_change(Step, 1);
    // if (Encoder == 2) // AF
    //     af_encoder_change(Step, 2);
    // if (Encoder == 3) // AGX
    //     rf_encoder_change(Step, 2);
    if (Encoder == 4) // SHIFT HI
        shiftHi_encoder_change(Step);
    if (Encoder == 5) // SHIFT LO
        shiftLo_encoder_change(Step);
    if (Encoder == 8) // XIT
        xit_encoder_change(Step);
    if (Encoder == 9) // RIT
        rit_encoder_change(Step);
    if (Encoder == 10) // DRIVE
        drive_encoder_change(Step);
    if (Encoder == 11) // MIKE
        mike_encoder_change(Step);
}

void af_encoder_change(int step, int radio)
{
    double gain = receiver[radio]->volume;
    gain += (double)step / 100.0;
    if (gain < 0.0)
    {
        gain = 0.0;
    }
    else if (gain > 1.0)
    {
        gain = 1.0;
    }
    set_af_gain(radio, gain);
}

void rf_encoder_change(int step, int radio)
{
    double gain = receiver[radio]->agc_gain;
    gain += (double)step;
    if (gain < -20.0)
    {
        gain = -20.0;
    }
    else if (gain > 120.0)
    {
        gain = 120.0;
    }
    set_agc_gain(radio, gain);
}

void drive_encoder_change(int step)
{
    double d = getDrive();
    d += (double)step / 100.0;
    set_drive(d);
}

void mike_encoder_change(int step)
{
    double gain = mic_gain;
    gain += (double)step / 100.0;
    if (gain < 0.0)
    {
        gain = 0.0;
    }
    else if (gain > 4.0)
    {
        gain = 4.0;
    }
    set_mic_gain(gain);
    // SetTXAPanelGain1(transmitter->id,pow(10.0, gain/20.0));
}

void rit_encoder_change(int step)
{
    vfo_rit(active_receiver->id, step++);
}

void xit_encoder_change(int step)
{
    transmitter->xit += (step * 10);
    vfo_update();
}

void shiftHi_encoder_change(int step)
{
    FILTER *band_filters = filters[vfo[active_receiver->id].mode];
    FILTER *band_filter = &band_filters[vfo[active_receiver->id].filter];
    band_filter->high += (step * 10);
    receiver_filter_changed(receiver[0]);
}

void shiftLo_encoder_change(int step)
{
    FILTER *band_filters = filters[vfo[active_receiver->id].mode];
    FILTER *band_filter = &band_filters[vfo[active_receiver->id].filter];
    band_filter->low += (step * 10);
    receiver_filter_changed(receiver[0]);
}

void RitXitButton(gboolean State, gboolean LongPress)
{
    if (State && !LongPress)
    {
        vfo_rit_update(active_receiver->id);
    }

    if (!State && LongPress)
    {
        transmitter->xit_enabled = transmitter->xit_enabled == 1 ? 0 : 1;
    }
}

void BandButton(int Button)
{
    switch (Button)
    {
    case 29:
        vfo_band_changed(active_receiver->id, band160);
        break;
    case 30:
        vfo_band_changed(active_receiver->id, band80);
        break;
    case 31:
        vfo_band_changed(active_receiver->id, band60);
        break;
    case 32:
        vfo_band_changed(active_receiver->id, band40);
        break;
    case 33:
        vfo_band_changed(active_receiver->id, band30);
        break;
    case 34:
        vfo_band_changed(active_receiver->id, band20);
        break;
    case 35:
        vfo_band_changed(active_receiver->id, band17);
        break;
    case 36:
        vfo_band_changed(active_receiver->id, band15);
        break;
    case 37:
        vfo_band_changed(active_receiver->id, band12);
        break;
    case 38:
        vfo_band_changed(active_receiver->id, band10);
        break;
    case 39:
        vfo_band_changed(active_receiver->id, band6);
        break;
    case 40:
        vfo_band_changed(active_receiver->id, bandGen);
        break;
    }
}

void FuncButton(int Button, gboolean State)
{
    if (State)
    {
        int i = Button - 20;
        fprintf(stderr, "%s: %d action=%d\n", __FUNCTION__, i, toolbar_switches[i].switch_function);
        PROCESS_ACTION *a = g_new(PROCESS_ACTION, 1);
        a->action = toolbar_switches[i].switch_function;
        a->mode = PRESSED;
        g_idle_add(process_action, a);
    }
    else
    {
        int i = Button - 20;
        fprintf(stderr, "%s: %d action=%d\n", __FUNCTION__, i, toolbar_switches[i].switch_function);
        PROCESS_ACTION *a = g_new(PROCESS_ACTION, 1);
        a->action = toolbar_switches[i].switch_function;
        a->mode = RELEASED;
        g_idle_add(process_action, a);
    }
}