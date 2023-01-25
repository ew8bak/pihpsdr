#ifndef ANDROMEDA_H
#define ANDROMEDA_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>

void parseAndromedaCommand(char *cmd);
void ZZZU(char *cmd);
void ZZZD(char *cmd);
void ZZZP(char *cmd);
void ZZZE(char *cmd);
void HandleFrontPanelButtonPress(int Button, gboolean State, gboolean LongPress);
void HandleFrontPanelEncoderStep(int Encoder, int Step);
void af_encoder_change(int step, int radio);
void rf_encoder_change(int step, int radio);
void drive_encoder_change(int step);
void mike_encoder_change(int step);
void rit_encoder_change(int step);
void xit_encoder_change(int step);
void shiftHi_encoder_change(int step);
void shiftLo_encoder_change(int step);
void RitXitButton(gboolean State, gboolean LongPress);
void BandButton(int Button);
void FuncButton(int Button, gboolean State);
void andromeda_send_resp(int fd, char *msg);

#endif // ANDROMEDA_H
