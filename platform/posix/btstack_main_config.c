/*
 * Copyright (C) 2025 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */

#define BTSTACK_FILE__ "btstack_main_config.c"

#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "btstack_audio.h"
#include "btstack_debug.h"
#include "btstack_memory.h"
#include "btstack_run_loop_posix.h"
#include "btstack_run_loop.h"
#include "btstack_stdin.h"
#include "btstack_util.h"
#include "hci_dump_posix_fs.h"
#include "hci_dump_posix_stdout.h"
#include "hci_transport.h"

static char short_options[] = "+hu:l:rb:a:f:";

static struct option long_options[] = {
    {"help",      no_argument,       NULL, 'h'},
    {"logfile",   required_argument, NULL, 'l'},
    {"logformat", required_argument, NULL, 'f'},
    {"reset-tlv", no_argument,       NULL, 'r'},
    {"tty",       required_argument, NULL, 'u'},
    {"bd-addr",   required_argument, NULL, 'm'},
    {"baudrate",  required_argument, NULL, 'b'},
    {0, 0, 0, 0}
};

static const char *help_options[] = {
    "print (this) help.",
    "set file to store debug output and HCI trace.",
    "set file format to store debug output in.",
    "reset bonding information stored in TLV.",
    "set path to Bluetooth Controller.",
    "set random static Bluetooth address.",
    "set initial baudrate.",
};

static const char *option_arg_name[] = {
    "",
    "LOGFILE",
    "btsnoop|bluez|pklg",
    "",
    "TTY",
    "BD_ADDR",
    "BAUDRATE",
};

static void usage(const char *name){
    unsigned int i;
    printf( "usage:\n\t%s [options]\n", name );
    printf("valid options:\n");
    for( i=0; long_options[i].name != 0; i++) {
        printf("--%-10s| -%c  %-10s\t\t%s\n", long_options[i].name, long_options[i].val, option_arg_name[i], help_options[i] );
    }
}

static const char *hci_dump_type_to_string[] = {
        [HCI_DUMP_INVALID]      = "invalid",
        [HCI_DUMP_BLUEZ]        = "bluez",
        [HCI_DUMP_PACKETLOGGER] = "pklg",
        [HCI_DUMP_BTSNOOP]      = "btsnoop",
};

#define STR(x) #x
#define ENUM_TO_STRING(x) [x] = STR(x)
static const char *hci_dump_enum_to_string[] = {
        ENUM_TO_STRING(HCI_DUMP_INVALID),
        ENUM_TO_STRING(HCI_DUMP_BLUEZ),
        ENUM_TO_STRING(HCI_DUMP_PACKETLOGGER),
        ENUM_TO_STRING(HCI_DUMP_BTSNOOP),
};

static int dump_format_name_to_enum(const char *name) {
    for( int i=HCI_DUMP_INVALID; i<=HCI_DUMP_BTSNOOP; ++i ) {
        if( !strcmp(hci_dump_type_to_string[i], name) ) {
            return i;
        }
    }
    return HCI_DUMP_INVALID;
}

static const char *get_file_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return NULL;
    return dot + 1;
}

int btstack_main_config(int argc, const char * argv[], hci_transport_config_uart_t *transport_config, bd_addr_t address, bool *tlv_reset ){
    btstack_assert(transport_config != NULL);
    const char * log_file_path = NULL;
    hci_dump_format_t dump_format = HCI_DUMP_PACKETLOGGER;
    int oldopterr = opterr;
    opterr = 0;
    // parse command line parameters
    while(true){
        int c = getopt_long( argc, (char* const *)argv, short_options, long_options, NULL );
        if (c < 0) {
            break;
        }
        if (c == '?'){
            continue;
        }
        switch (c) {
            case 'u':
                transport_config->device_name = optarg;
                break;
            case 'l':
                log_file_path = optarg;
                break;
            case 'r':
                if( tlv_reset != NULL ) {
                    *tlv_reset = true;
                }
                break;
            case 'm':
                if( address != NULL ) {
                    sscanf_bd_addr(optarg, address);
                }
                break;
            case 'b':
                transport_config->baudrate_init = atoi( optarg );
                break;
            case 'f':
                dump_format = dump_format_name_to_enum( optarg );
                break;
            case 'h':
            default:
                usage(argv[0]);
                break;
        }
    }
    // reset getopt parsing, so it works as intended from btstack_main
    optind = 1;
    opterr = oldopterr;

    /// GET STARTED with BTstack ///
    btstack_memory_init();
    btstack_run_loop_init(btstack_run_loop_posix_get_instance());

    // determine packet log file name
    char pklg_path[PATH_MAX] = "/tmp/hci_dump_";
    if (log_file_path == NULL){
        char log_postfix[9] = ".";
        if( dump_format == HCI_DUMP_INVALID ) {
            dump_format = HCI_DUMP_PACKETLOGGER;
        }
        btstack_strcat( log_postfix, sizeof(log_postfix), hci_dump_type_to_string[dump_format] );

        char *app_name = strndup( argv[0], PATH_MAX );
        char *device_path = strndup( transport_config->device_name, PATH_MAX );
        char *base_name = basename( app_name );
        btstack_strcat( pklg_path, sizeof(pklg_path), base_name );
        btstack_strcat( pklg_path, sizeof(pklg_path), "_" );

        char *device = basename(device_path);
        for(unsigned i=0; i<strlen(device); ++i) {
            if(device[i] == '.') {
                device[i] = '_';
            }
        }
        btstack_strcat( pklg_path, sizeof(pklg_path), device );
        btstack_strcat( pklg_path, sizeof(pklg_path), log_postfix );

        free( app_name );
        free( device_path );
        log_file_path = pklg_path;
    } else {
        // try to guess type from file extension
        const char *ext = get_file_ext(log_file_path);
        if( ext != NULL ) {
            dump_format = dump_format_name_to_enum( ext );
        }
        if( dump_format == HCI_DUMP_INVALID ) {
            dump_format = HCI_DUMP_PACKETLOGGER;
        }
    }
    hci_dump_posix_fs_open(log_file_path, dump_format);
    const hci_dump_t * hci_dump_impl = hci_dump_posix_fs_get_instance();
    hci_dump_init(hci_dump_impl);
    printf("Packet Log: %s\n", log_file_path);
    printf("Log format: %s\n", hci_dump_enum_to_string[dump_format]);
    printf("Device    : \"%s\"\n", transport_config->device_name);
    printf("Baudrate  : %d\n", transport_config->baudrate_init);
    if((tlv_reset != NULL) && *tlv_reset) {
        printf("Reset tlv : true");
    }
    if((address != NULL) && !btstack_is_null_bd_addr(address)) {
        printf("address   : %s\n", bd_addr_to_str(address));
    }
#ifdef HAVE_PORTAUDIO
    btstack_audio_sink_set_instance(btstack_audio_portaudio_sink_get_instance());
    btstack_audio_source_set_instance(btstack_audio_portaudio_source_get_instance());
#endif

    return EXIT_SUCCESS;
}
