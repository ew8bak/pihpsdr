#include <gtk/gtk.h>
#include <fcntl.h>
#include <stdio.h>
#include "receiver.h"
#include "toolbar.h"
#include "band_menu.h"
#include "sliders.h"
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
#include "store.h"
#include "ext.h"
#include "noise_menu.h"
#include "actions.h"
#include "andromeda.h"
#include "andromeda_menu.h"
#include <termios.h>

typedef struct _client
{
    int fd;
    GThread *thread_id;
} CLIENT;

typedef struct _command
{
    CLIENT *client;
    char *command;
} COMMAND;

#define MAXDATASIZE 2000

gboolean implemented;

typedef struct
{
    GMutex m;
} GT_MUTEX;

GT_MUTEX *mutex_andromeda;
GT_MUTEX *mutex_andromeda_busy;
static GThread *andromeda_serial_server_thread_id = NULL;
static gboolean andromeda_serial_running = FALSE;
int mutex_andromeda_exists = 0;
int andromeda_fd;

void parseAndromedaCommand(void *data)
{
    COMMAND *info = (COMMAND *)data;
    CLIENT *client = info->client;
    char *command = info->command;
    char reply[80];
    reply[0] = '\0';
    if (command[2] == 'Z')
    {
        switch (command[3])
        {
        case 'U': // ZZZU, VFO Encoder Up
            ZZZU(command);
            break;
        case 'D': // ZZZD, VFO Encoder Down
            ZZZD(command);
            break;
        case 'P': // ZZZP, Button
            ZZZP(command);
            break;
        case 'E': // ZZZP, Encoder
            ZZZE(command);
            break;
        case 'S':
            ZZZS(command);
            break;
        default:
            implemented = FALSE;
            break;
        }
    }
}

void ZZZS(char *cmd)
{
    printf(cmd);
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

static gpointer andromeda_serial_server(gpointer data)
{
    CLIENT *client = (CLIENT *)data;
    char cmd_input[MAXDATASIZE];
    char *command = g_new(char, MAXDATASIZE);
    int command_index = 0;
    int numbytes;
    int i;
    andromeda_serial_running = TRUE;
    while (andromeda_serial_running)
    {
        numbytes = read(andromeda_fd, cmd_input, sizeof cmd_input);
        if (numbytes > 0)
        {
            for (i = 0; i < numbytes; i++)
            {
                command[command_index] = cmd_input[i];
                command_index++;
                if (cmd_input[i] == ';')
                {
                    command[command_index] = '\0';
                    if (andromeda_debug)
                        g_print("Andromeda: command=%s\n", command);
                    COMMAND *info = g_new(COMMAND, 1);
                    info->client = client;
                    info->command = command;
                    g_mutex_lock(&mutex_andromeda_busy->m);
                    g_idle_add(parseAndromedaCommand, info);
                    g_mutex_unlock(&mutex_andromeda_busy->m);

                    command = g_new(char, MAXDATASIZE);
                    command_index = 0;
                }
            }
        }
        else if (numbytes < 0)
        {
            break;
        }
    }
    close(andromeda_fd);
    return NULL;
}

int launch_serial_andromeda()
{
    g_print("Andromeda: Launch Serial port %s\n", andromeda_serial_port);
    if (mutex_andromeda_exists == 0)
    {
        mutex_andromeda = g_new(GT_MUTEX, 1);
        g_mutex_init(&mutex_andromeda->m);
        mutex_andromeda_exists = 1;
    }

    if (mutex_andromeda_busy == NULL)
    {
        mutex_andromeda_busy = g_new(GT_MUTEX, 1);
        g_print("Andromeda: mutex_andromeda_busy=%p\n", mutex_andromeda_busy);
        g_mutex_init(&mutex_andromeda_busy->m);
    }

#ifdef _WIN32
    andromeda_fd = _open(andromeda_serial_port, O_RDWR);
#else
    andromeda_fd = open(andromeda_serial_port, O_RDWR | O_NOCTTY | O_SYNC);
#endif

    if (andromeda_fd < 0)
    {
        g_print("Andromeda: Error %d opening %s: %s\n", errno, andromeda_serial_port, strerror(errno));
        return 0;
    }

    g_print("Andromeda serial port andromeda_fd=%d\n", andromeda_fd);

    set_andromeda_interface_attribs(andromeda_fd, andromeda_serial_baud_rate, andromeda_serial_parity);
    set_andromeda_blocking(andromeda_fd, 1);

    andromeda_serial_server_thread_id = g_thread_new("Andromeda serial server", andromeda_serial_server, NULL);
    if (!andromeda_serial_server_thread_id)
    {
        g_print("g_thread_new failed on andromeda_serial_server\n");
        return 0;
    }
    andromeda_send_resp(andromeda_fd, "ZZZS;");
    return 1;
}

void andromeda_disable_serial()
{
    g_print("Andromeda: Disable Serial port %s\n", andromeda_serial_port);
    andromeda_serial_running = FALSE;
}

void andromeda_send_resp(int fd, char *msg)
{
    if (andromeda_debug)
        g_print("Andromeda: RESP=%s\n", msg);
    int length = strlen(msg);
    int rc;
    int count = 0;

    while (length > 0)
    {
        rc = write(fd, msg, length);
        if (rc < 0)
            return;
        if (rc == 0)
        {
            count++;
            if (count > 10)
                return;
        }
        length -= rc;
        msg += rc;
    }
}

int set_andromeda_interface_attribs(int fd, int speed, int parity)
{
#ifdef _WIN32
    ///???
#else
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0)
    {
        g_print("Andromeda: Error %d from tcgetattr", errno);
        return -1;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK; // disable break processing
    tty.c_lflag = 0;        // no signaling chars, no echo,
                            // no canonical processing
    tty.c_oflag = 0;        // no remapping, no delays
    tty.c_cc[VMIN] = 0;     // read doesn't block
    tty.c_cc[VTIME] = 5;    // 0.5 seconds read timeout

    // tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
    tty.c_iflag |= (IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);   // ignore modem controls,
                                       // enable reading
    tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        g_print("Andromeda: Error %d from tcsetattr", errno);
        return -1;
    }
#endif
    return 0;
}

void set_andromeda_blocking(int fd, int should_block)
{
#ifdef _WIN32
    ///???
#else
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0)
    {
        g_print("Andromeda: Error %d from tggetattr\n", errno);
        return;
    }
    tty.c_cc[VMIN] = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5; // 0.5 seconds read timeout

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
        g_print("Andromeda: error %d setting term attributes\n", errno);
#endif
}