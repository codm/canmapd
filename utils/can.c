/*
   CAN helper functions for ibdoor C daemon
*/

void interpret_canmessage(ACM_CANMESSAGE *msg)
{
    if(msg->receiver == 0xFE)
    {
        if(msg->sender < 0xF0 && msg->command < 0x08)
        {
            printf("Hier Dings\n");
        }
    }
}

void translate_frame(CANFRAME *canframe, ACM_CANMESSAGE *msg)
{
    int i;
    msg->receiver = canframe->can_id;
    msg->sender = canframe->data[0];
    msg->command = canframe->data[1];
    for(i = 0; i <= 5; i++)
    {
        msg->data[i] = canframe->data[i+2];
    }
}

