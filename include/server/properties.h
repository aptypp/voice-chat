//
// Created by artur on 01.11.23.
//

#ifndef VOICE_CHAT_PROPERTIES_H
#define VOICE_CHAT_PROPERTIES_H

typedef struct ServerProperties
{
    uint32_t max_clients_count;
    uint32_t sleep_time_seconds;
    uint16_t port;

} ServerProperties;


#endif //VOICE_CHAT_PROPERTIES_H
