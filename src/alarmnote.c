#define OFFSET 25 /* Used for Pebble Round */

/*
 * raj - 7/2015 - changed to "AlarmNote" ver. 1.0
 * 
 * This watch face was created by starting with the "Gatekeeper" watch face by Tycho.
 * It has been somewhat modified and I tried to remove the layers and variables, etc.,
 * which aren't being used now.
 */

#include <pebble.h>

/* My version numbers */
#define MAJOR	0x1
#define MINOR	0x0

#define MAX_MSG_LEN	60
/*
 * These are not only used to index into the "my_date_layer" array
 * but they also have to match the name to number specifications in
 * the appinfo.json file, so that they can used as keys for
 * datebase lookups.
 */
#define FONTSIZE	1
#define DISABLE		2
#define A0		7
#define MESS0		8
#define A1		9
#define MESS1		10
#define A2		11
#define MESS2		12
#define A3		13
#define MESS3		14
#define A4		15
#define MESS4		16
#define A5		17
#define MESS5		18
#define A6		19
#define MESS6		20
#define SNOOZEID	21
#define SNOOZEMSG	22
#define SNOOZETIME	23
#define APPSNOOZEID	24
#define AID0		25
#define AID1		26
#define AID2		27
#define AID3		28
#define AID4		29
#define AID5		30
#define AID6		31
#define ATIME0		32
#define ATIME1		33
#define ATIME2		34
#define ATIME3		35
#define ATIME4		36
#define ATIME5		37
#define ATIME6		38


#define MAX_ALARMS	8
int time_idx[MAX_ALARMS]={A0, A1, A2, A3, A4, A5, A6};
int mess_idx[MAX_ALARMS]={MESS0, MESS1, MESS2, MESS3, MESS4, MESS5, MESS6};
int alarmid_idx[MAX_ALARMS]={AID0, AID1, AID2, AID3, AID4, AID5, AID6};
int alarmtime_idx[MAX_ALARMS]={ATIME0, ATIME1, ATIME2, ATIME3, ATIME4,
                               ATIME5, ATIME6};
#define MAX_MESS_LEN	80

bool got_connections;

/*
 * Font sizes we know about
 */
struct font_info_ 
{
    char *pebble_font;
    int font_size;
} Pebble_fonts[]={{FONT_KEY_GOTHIC_24_BOLD, 24},
                  {FONT_KEY_BITHAM_30_BLACK, 30},
                  {FONT_KEY_GOTHIC_28_BOLD, 28},
		  {FONT_KEY_DROID_SERIF_28_BOLD, 29}};
      
int Font_size;    


time_t snooze_time;
AppTimer *app_snooze_id;
WakeupId snooze_id;
uint snooze_msgid;


static GFont note_font;
//static Window *my_window;
static Layer *my_window_layer;

static TextLayer *note_layer;
static Window *note_window;
SimpleMenuLayer *menu_layer;

static Window *menu_window;
static Window *my_menu_window;

static Window *alarm_window=NULL;
static SimpleMenuLayer *alarm_win_layer=NULL;

/* These are used for random notes */
static Window *notewin = NULL;
static TextLayer *notelayer = NULL;

time_t last_note_time;
int last_note_alarm;
int32_t last_wakeup_cookie;
WakeupId last_wakeup_id;
time_t last_wakeup_time;

status_t stat;

static bool bluetooth_connected;

char text_message[120];

#define SECONDS (1)
#define MINUTES (60 * SECONDS)
#define HOURS (60 * MINUTES)
#define DAYS (24 * HOURS)

#if defined(PBL_RECT)
#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168
#elif defined(PBL_ROUND)
#define SCREEN_WIDTH 180
#define SCREEN_HEIGHT 180
#endif

//static char *Day_names[]={
//    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
//};

static const uint32_t const alert_segments[]={
    500, 100, 200, 100, 500, 100, 200, 100, 500
};
VibePattern alert_pulse = {
    .durations = alert_segments,
    .num_segments = ARRAY_LENGTH(alert_segments),
};

/* Messages */
char *mess[MAX_ALARMS]={NULL, NULL, NULL, NULL, NULL, NULL, NULL};
int alarm_hours[MAX_ALARMS];
int alarm_mins[MAX_ALARMS];
WakeupId alarm_id[MAX_ALARMS];
WakeupId alarm_time[MAX_ALARMS];
time_t alarm_missed[MAX_ALARMS];
WakeupId last_wakeup_handled[MAX_ALARMS];
int alarm_current;
time_t now;
time_t last_vibes;                      /* last time vibration was done */
bool dirty_pins=false;
bool alarms_disabled=false;             /* alarms currently disabled */

// void snooze_callback(void *data);

void
display_pill_reminder (int alarm_num) 
{
    char *src, *dst;

    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
	    "Notification number %d", alarm_num);

    /* If the message has disappeared, exit now */
    if (!mess[alarm_num] || !*mess[alarm_num])
	return;

/*
 * If the window is already being displayed, clear it for redisplay
 */
    if (window_stack_contains_window(note_window)) {
	window_stack_pop(true);
    }
    
    dst = text_message;
    
    /* Note if this was a missed alarm */
    if ((int32_t)alarm_missed[alarm_num] != 0) {
	*dst++ = '*';
    }

    for (src = mess[alarm_num];
	 *src; dst++ , src++) {
	*dst = *src;
    }
    *dst = '\0';
    alarm_missed[alarm_num] = false;
    text_layer_set_font(note_layer, note_font);
    text_layer_set_text(note_layer, text_message);
    window_stack_push(note_window, true);

    time(&last_note_time);
    last_note_alarm = alarm_num;

    vibes_enqueue_custom_pattern(alert_pulse);
    last_vibes = last_note_time;
}


/*************************************
 * Note menu definitions
 */
void
menu_select_click_handler (ClickRecognizerRef recognizer, void *context) 
{

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "Menu Click handler");

}


void handle_wakeup(WakeupId wakeup_id, int32_t cookie);

void
note_sleep_callback (void* cookie) {

    handle_wakeup(time(0L), (int32_t)cookie);
}

void
create_and_push_text_window(char *text, GFont font) 
{
    if (!notewin) {
	notewin = window_create();
//	window_set_fullscreen(notewin, true);
#if defined(PBL_RECT)
	notelayer = text_layer_create(GRect(0,0,144,168));
#elif defined(PBL_ROUND)
        Layer *window_layer = window_get_root_layer(notewin);
        GRect bounds = layer_get_bounds(window_layer);
        notelayer = text_layer_create(GRect(OFFSET,OFFSET,bounds.size.w-(OFFSET*2),bounds.size.h-(OFFSET*2)));
        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,
                "set for round\n");
#endif
	text_layer_set_overflow_mode(notelayer, GTextOverflowModeWordWrap);
	layer_add_child(window_get_root_layer(notewin), text_layer_get_layer(notelayer));
    }

    text_layer_set_text_alignment(notelayer, GTextAlignmentCenter);
    text_layer_set_font(notelayer, font);
    text_layer_set_text(notelayer, text);

    window_stack_push(notewin, true);
}

WakeupId
schedule_my_wakeup (time_t alarm_time,
                    int alarm_num,
                    char *emessage, time_t *actual_time) 
{
    WakeupId wake_id;
    
    /* Schedule a new (snooze) wakeup */
    for (wake_id = E_RANGE ; wake_id == E_RANGE ; alarm_time -= (1 * MINUTES)) {
        wake_id = wakeup_schedule(alarm_time,
                                  alarm_num,
                                  true);
        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,
                "My wakeup - Alarm for time %u (context=%d) set with id %d",
                (uint)alarm_time, alarm_num, (int)wake_id);
        if (wake_id > 0) break;
    }

    if (wake_id < 0) {
        snprintf(text_message, sizeof(text_message),
                 "Error scheduling snooze - %d", (int)wake_id);
        create_and_push_text_window(text_message, note_font);
    } else {
        *actual_time = alarm_time;
    }

    return(wake_id);
}

    

void
menu_callback (int index, void *context) 
{
    time_t	t, new_t;

    t = now;

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "Menu callback on row %d, now=%u\n", index, (int)t);

    t += ((index+1)*5) * 60;
    t = t - (t % 60);                   /* remove seconds */

    snooze_id = schedule_my_wakeup(t,
                                   alarm_current+1+100, /* signal a snooze */
                                   "Error scheduling snooze - %d", &new_t);
//    snooze_id = wakeup_schedule(t,
//                                alarm_current+1 + 100 /* signal it's a snooze */,
//                                false);

    t = new_t;
    snooze_time = t;
    snooze_msgid = alarm_current;
    alarm_time[7] = t;
//    app_snooze_id = app_timer_register((t - now) * 1000,
//                                       snooze_callback,
//                                       (void *)alarm_current+1 + 100);
    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "Scheduling snooze at %u, context=%d\n",
	    (int)t, (int)alarm_current+1);
    persist_write_int(SNOOZEID, (uint32_t)snooze_id);
    persist_write_int(APPSNOOZEID, (uint32_t)app_snooze_id);
    persist_write_int(SNOOZEMSG, (uint32_t)snooze_msgid);
    persist_write_int(SNOOZETIME, (uint32_t)snooze_time);

    window_stack_pop(true); /* menu window */
    window_stack_pop(true); /* note window */
}

/*
 * Notification window menu
 */
const SimpleMenuItem menu_items[]={
    {"5 min.", NULL, NULL, (SimpleMenuLayerSelectCallback)menu_callback},
    {"10 min.", NULL, NULL, (SimpleMenuLayerSelectCallback)menu_callback},
    {"15 min.", NULL, NULL, (SimpleMenuLayerSelectCallback)menu_callback},
    {"20 min.", NULL, NULL, (SimpleMenuLayerSelectCallback)menu_callback},
    {"25 min.", NULL, NULL, (SimpleMenuLayerSelectCallback)menu_callback},
    {"30 min.", NULL, NULL, (SimpleMenuLayerSelectCallback)menu_callback},
};
#define num_menu_items 6

const SimpleMenuSection menu_sections={.title="Sleep time", .items=menu_items, .num_items=num_menu_items};
#define num_menu_sections 1

SimpleMenuLayer *my_menu_layer;


void
log_alarms (void) 
{
    int i;

    for (i = 0 ; i < MAX_ALARMS ; i++) {
	if (mess[i]) {
	    app_log(APP_LOG_LEVEL_WARNING,
		    __FILE__,
		    __LINE__,
		    "Alarm at %d:%d: '%s'",
		    alarm_hours[i], alarm_mins[i], mess[i]);
	} else {
	    app_log(APP_LOG_LEVEL_WARNING,
		    __FILE__,
		    __LINE__,
		    "Alarm at %d:%d: (none)",
		    alarm_hours[i], alarm_mins[i]);
	}
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,
		"Alarm ID index %d is %u last handled at %u",
		i, (uint)alarm_id[i], (uint)last_wakeup_handled[i]);
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,
		"Alarm ID index %d is %u at %u",
		i, (uint)alarm_id[i], (uint)alarm_time[i]);
	if (alarm_missed[i]) {
	    app_log(APP_LOG_LEVEL_WARNING,
		    __FILE__,
		    __LINE__,
		    "Alarm ID index %d was missed at %u",
		    i, (uint)alarm_missed[i]);
	}
	alarm_missed[i] = 0;
    }
}



/*
 * Returns index of the next alarm - MAX_ALARMS if there are none.
 */
int
find_next_alarm(void) 
{
    int i;
    int alarm_num;
    unsigned int alarm_diff;

    alarm_diff = (unsigned int)0xFFFFFFFF; /* init. */
    alarm_num = MAX_ALARMS;

    for (i = 0 ; i < MAX_ALARMS-1 ; i++) {
	if (mess[i] && *mess[i]) {
	    if ((unsigned int)(alarm_time[i] - now) < alarm_diff) {
		app_log(APP_LOG_LEVEL_WARNING,
			__FILE__,
			__LINE__,
			"found alarm_id[%d] as next one\n", i);
		alarm_diff = (unsigned int)(alarm_time[i] - now);
		alarm_num = i;
	    }
	}
    }

    return (alarm_num);			/* even if it wasn't reset */
}

void
cancel_alarm(int idx) 
{
    wakeup_cancel(alarm_id[idx]);
    alarm_id[idx] = 0;
    alarm_time[idx] = 0;
    persist_write_int(alarmid_idx[idx], (uint32_t)alarm_id[idx]);
    persist_write_int(alarmtime_idx[idx], (uint32_t)alarm_time[idx]);
}

void
cancel_snooze(void) 
{
    wakeup_cancel(snooze_id);
    snooze_id = 0;
    snooze_time = 0;
    snooze_msgid = 0;
//    app_timer_cancel(app_snooze_id);
    app_snooze_id = NULL;
    persist_write_int(SNOOZEID, (uint32_t)snooze_id);
    persist_write_int(APPSNOOZEID, (uint32_t)app_snooze_id);
    persist_write_int(SNOOZEMSG, (uint32_t)snooze_msgid);
    persist_write_int(SNOOZETIME, (uint32_t)snooze_time);
}


void
update_pins (int pin) 
{
    DictionaryIterator *iter;
    int i;
    AppMessageResult rc;
    
    if (!got_connections) {
        return;
    }

    if (!dirty_pins) {
        return;
    }
    
    /*
     * Update timeline pins
     */
    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "\n\n======== Update Pins - Begin writing to JS");
    rc = app_message_outbox_begin(&iter);
    if (!iter) {
        // Error creating outbound message
        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,
                "Error writing to JS = 0x%x", (uint)rc);
        return;
    }
    
    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "Pin = %d", (int)pin);

    for (i = 0 ; i < MAX_ALARMS-1 ; i++) {
        int value;

        if (pin == 0 || pin == i) {
            value = alarm_time[i];
            dict_write_int(iter, time_idx[i], &value, sizeof(int), false);
            
            if (mess[i] && *mess[i] && !alarms_disabled) {
                app_log(APP_LOG_LEVEL_WARNING,
                        __FILE__,
                        __LINE__,
                        "Message %d = '%s'", i, mess[i]);
                dict_write_cstring(iter, mess_idx[i], (const char *)mess[i]);
            } else {
                dict_write_cstring(iter, mess_idx[i], (const char *)"");
            }
        }
    }
    dict_write_end(iter);
    app_message_outbox_send();
    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "Done writing to JS");

    dirty_pins = false;
}

/*
 * Find and reset next alarm
 * Reset it for 24 hours later
 */
void
skip_next_alarm(void) {
    int alarm_num;
    int days;
    time_t a_time, new_time;

    alarm_num = find_next_alarm();

    if (alarm_num == MAX_ALARMS) {
	return;				/* no alarms */
    }

    /* Now reschedule it */
    a_time = alarm_time[alarm_num];
    cancel_alarm(alarm_num);
    alarm_time[alarm_num] = a_time + (1 * DAYS);
    alarm_id[alarm_num] = schedule_my_wakeup((int)alarm_time[alarm_num],
                                             alarm_num+1,
                                             "Error scheduling "
                                             "an alarm - %d", &new_time);
//    alarm_id[alarm_num] = wakeup_schedule((int)alarm_time[alarm_num],
//					  alarm_num+1, true);

    alarm_time[alarm_num] = new_time;   /* save actual time scheduled */
    persist_write_int(alarmid_idx[alarm_num], (uint32_t)alarm_id[alarm_num]);
    persist_write_int(alarmtime_idx[alarm_num], (uint32_t)alarm_time[alarm_num]);
    dirty_pins = true;
    update_pins(alarm_num);

    days = (alarm_time[alarm_num] - now) / (1 * DAYS);
    snprintf(text_message, sizeof(text_message),
             "Rescheduled %2d:%02d alarm for +%d day%c",
             alarm_hours[alarm_num], alarm_mins[alarm_num],
             days, (days > 1) ? 's' : ' ');
    
    create_and_push_text_window(text_message, note_font);
}


/*
 * show next alarm
 * Reset it for 24 hours later
 */
void
show_next_alarm(void) {
    int alarm_num;
    int days;

    if (alarms_disabled) {
        snprintf(text_message, sizeof(text_message),
                 "Alarms are disabled");
        create_and_push_text_window(text_message, note_font);
        return;
    }

    alarm_num = find_next_alarm();

    if (alarm_num == MAX_ALARMS) {
	return;				/* no alarms */
    }

    days = (alarm_time[alarm_num] - now) / (1 * DAYS);
    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "%d days out\n", days);

    if (days > 0) {
        snprintf(text_message, sizeof(text_message),
                 "%d:%02d +%d day%c\n%s",
                 alarm_hours[alarm_num], alarm_mins[alarm_num],
                 days,
                 (days > 1) ? 's' : ' ',
                 mess[alarm_num]);

    } else {
        snprintf(text_message, sizeof(text_message), "%d:%02d\n%s",
                 alarm_hours[alarm_num], alarm_mins[alarm_num],
                 mess[alarm_num]);
    }

    create_and_push_text_window(text_message, note_font);
}


void
schedule_alarms(time_t now, struct tm* now_tick, bool schedule_it) 
{
    int i;
    time_t alarm_tim;
    int32_t time_inc;

    for (i = 0 ; i < MAX_ALARMS-1 ; i++) {
        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,
                "Scheduling alarm[%d] =============", i);
	if (mess[i] && *mess[i]) {
//	    app_log(APP_LOG_LEVEL_WARNING,
//		    __FILE__,
//		    __LINE__,
//		    "handling alarm %d\n", i);
//	    app_log(APP_LOG_LEVEL_WARNING,
//		    __FILE__,
//		    __LINE__,
//		    "making time for alarm %d\n", i);

            alarm_tim = alarm_time[i];
//            if (alarm_tim == 0) {
                time_inc = ((alarm_hours[i] - now_tick->tm_hour) * HOURS) +
                    ((alarm_mins[i] - now_tick->tm_min) * MINUTES) -
                    now_tick->tm_sec;
                
                alarm_tim = now + time_inc;
                
                if (alarm_tim < now) {
                    app_log(APP_LOG_LEVEL_WARNING,
                            __FILE__,
                            __LINE__,
                            "alarm %d is for tomorrow %u", i,
                            (unsigned int)alarm_tim);
                    alarm_tim += 1 * DAYS;
                }
//            }
            
	    if (schedule_it) {
                time_t new_t;
                
                alarm_id[i] = schedule_my_wakeup(alarm_tim,
                                                 i+1,
                                                 "Error scheduling "
                                                 "an alarm - %d", &new_t);
//		alarm_id[i] = wakeup_schedule(alarm_tim, i+1, true);
                alarm_tim = new_t;      /* reset to actual time scheduled */
                alarm_time[i] = alarm_tim;
                persist_write_int(alarmid_idx[i],
                                  (uint32_t)alarm_id[i]);
                persist_write_int(alarmtime_idx[i],
                                  (uint32_t)alarm_time[i]);
		app_log(APP_LOG_LEVEL_WARNING,
			__FILE__,
			__LINE__,
			"scheduled for %u", (uint)alarm_tim);
	    } else {
		alarm_time[i] = alarm_tim;
                alarm_id[i] = 0;        /* don't have this */
		app_log(APP_LOG_LEVEL_WARNING,
			__FILE__,
			__LINE__,
			"setting");
	    }

	    if (alarm_id[i] < 0) {
		app_log(APP_LOG_LEVEL_WARNING,
			__FILE__,
			__LINE__,
			"alarm_id[%d] returned error %d\n", i,
			(int)alarm_id[i]);
		alarm_id[i] = 0;	/* reset */
	    }
	}
    }

    /* Need to make sure our snooze timer is set as well */
    if (snooze_time > now) {
        time_t new_time;
        snooze_id = schedule_my_wakeup(snooze_time,
                                       snooze_msgid+1+100,
                                       "Error scheduling snooze - %d", &new_time);
//        snooze_id = wakeup_schedule(snooze_time,
//                                    snooze_msgid+1 + 100 /* signal it's a snooze */,
//                                    false);
        snooze_time = new_time;
        if (snooze_id == -1) {
            snooze_id = 0;
        } else {
            app_log(APP_LOG_LEVEL_WARNING, __FILE__, __LINE__,
                    "Scheduling snooze wakup in %u seconds\n",
                    (uint)(snooze_time - now));
//            app_snooze_id = app_timer_register((snooze_time - now) * 1000,
//                                               snooze_callback,
//                                               (void *)snooze_msgid+1+100);
        }
    }

    dirty_pins = true;
    update_pins(0);                 /* update timeline pins */
}


void
reset_alarms (void) 
{
    int i;
    struct tm *now_tick;

    alarms_disabled = false;
    persist_write_bool(DISABLE, alarms_disabled);

    for (i = 0 ; i < MAX_ALARMS-1 ; i++) {
	if (alarm_id[i])
	    cancel_alarm(i);
    }

    now_tick = localtime(&now);
    schedule_alarms(now, now_tick, true);

    snprintf(text_message, sizeof(text_message),
             "Alarms are reset");

    create_and_push_text_window(text_message, note_font);
    dirty_pins = true;
    update_pins(0);
}


void
disable_alarms (void) 
{
    int i;

    wakeup_cancel_all();                /* cancel everything */

    alarms_disabled = true;
    persist_write_bool(DISABLE, alarms_disabled);

    for (i = 0 ; i < MAX_ALARMS ; i++) {
	alarm_id[i] = 0;		/* init. */
	alarm_missed[i] = 0;
	last_wakeup_handled[i] = 0;
    }

    snprintf(text_message, sizeof(text_message),
             "Alarms are disabled");
    create_and_push_text_window(text_message, note_font);

    dirty_pins = true;
    update_pins(0);
}

SimpleMenuItem alarms[MAX_ALARMS];
int alarms_idx[MAX_ALARMS];
char alarm_times[MAX_ALARMS][15];
SimpleMenuSection alarm_menu_sections;

void
show_an_alarm (int index, void *context) 
{

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "Show alarm %d\n", index);

    create_and_push_text_window(mess[alarms_idx[index]], note_font);
}


void
show_alarms (void)
{
    int i, j;
    int days;

    for (i = 0 , j = 0 ; i < MAX_ALARMS-1 ; i++) {
	if (!mess[i] || !*mess[i])
	    continue;

        days = (alarm_time[i] - now) / (1 * DAYS);
        if (days == 0) {
            snprintf(&alarm_times[j][0], sizeof(alarm_times[j]),
                     "%s%2d:%02d%s",
                     alarms_disabled ? "(" : "",
                     alarm_hours[i], alarm_mins[i],
                     alarms_disabled ? ")" : ""
                );
        } else {
            snprintf(&alarm_times[j][0], sizeof(alarm_times[j]),
                     "%s%2d:%02d +%d%s",
                     alarms_disabled ? "(" : "",
                     alarm_hours[i], alarm_mins[i],
                     days,
                     alarms_disabled ? ")" : ""
                );
        }
        
	alarms[j].title = alarm_times[j];
//	alarms[j].subtitle = (char *)i;
	alarms[j].subtitle = NULL;
	alarms[j].icon = NULL;
	alarms[j].callback = show_an_alarm;
	alarms_idx[j] = i;
	j++;
    }
    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "Made alarm menu with %d items\n", j);

    alarm_menu_sections.title="Alarms";
    alarm_menu_sections.items=alarms;
    alarm_menu_sections.num_items=j;

    if (!alarm_window) {
	alarm_window = window_create();
//	window_set_fullscreen(alarm_window, false);
//	window_set_click_config_provider(alarm_window, (ClickConfigProvider)alarm_config_provider);
    }
    if (alarm_win_layer) {
	simple_menu_layer_destroy(alarm_win_layer);
    }
    alarm_win_layer = simple_menu_layer_create(layer_get_frame(window_get_root_layer(my_menu_window)),
                                               alarm_window, &alarm_menu_sections, 1, NULL);
    layer_add_child(window_get_root_layer(alarm_window), simple_menu_layer_get_layer(alarm_win_layer));
    window_stack_push(alarm_window, true);
}



/*************************************
 * Main menu definitions
 */
void
my_menu_callback (int index, void *context) 
{
    
    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "Main Menu callback on row %d\n", index);

    switch ( index ) {
    case 0:				/* cancel snooze */
	cancel_snooze();
	break;

    case 1:				/* show next alarm */
	show_next_alarm();
	return;

    case 2:				/* skip next alarm */
	skip_next_alarm();
	return;

    case 3:				/* show alarms */
	show_alarms();
	return;

    case 4:				/* reset alarms */
	reset_alarms();
	return;

    case 5:				/* disable alarms */
	disable_alarms();
	return;
        

#ifdef notdef
    case 5:				/* debug alarms */
	log_alarms();
	break;

    case 6:				/* debug last note time */
	tm = localtime(&last_note_time);
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,
		"Last note reminder was %d at %d/%d %d:%0d",
		last_note_alarm,
		tm->tm_mon+1, tm->tm_mday,
		tm->tm_hour, tm->tm_min);
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,
		"Last wakeup id was %u with cookie %d at time %u",
		(uint)last_wakeup_id,
                (int)last_wakeup_cookie,
                (uint)last_wakeup_time);
	break;
#endif	
    }
    
    window_stack_pop(true); /* menu window */
}


/*
 * Main window menu
 */
const SimpleMenuItem my_menu_items[]={
    {"Cancel snooze", NULL, NULL, (SimpleMenuLayerSelectCallback)my_menu_callback},
    {"Next alarm", NULL, NULL, (SimpleMenuLayerSelectCallback)my_menu_callback},
    {"Skip next alarm", NULL, NULL, (SimpleMenuLayerSelectCallback)my_menu_callback},
    {"Show alarms", NULL, NULL, (SimpleMenuLayerSelectCallback)my_menu_callback},
    {"Restore alarms", NULL, NULL, (SimpleMenuLayerSelectCallback)my_menu_callback},
    {"Disable alarms", NULL, NULL, (SimpleMenuLayerSelectCallback)my_menu_callback},
//    {"Debug alarms", NULL, NULL, (SimpleMenuLayerSelectCallback)my_menu_callback},
//    {"Debug note", NULL, NULL, (SimpleMenuLayerSelectCallback)my_menu_callback},
};
#define num_my_menu_items (sizeof(my_menu_items) / sizeof(*my_menu_items))

const SimpleMenuSection my_menu_sections={.title="Main menu", .items=my_menu_items, .num_items=num_my_menu_items};
#define num_my_menu_sections 1


void
menu_config_provider (Window *window) 
{

    window_single_click_subscribe(BUTTON_ID_SELECT, menu_select_click_handler);
}



void
note_select_click_handler (ClickRecognizerRef recognizer, void *context) 
{

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "Note Click handler");

    window_stack_push(menu_window, true);
    simple_menu_layer_set_selected_index(menu_layer, 1 /* 10 min */, false);
}

void
my_select_click_handler (ClickRecognizerRef recognizer, void *context) 
{

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "My Click handler");

    window_stack_push(my_menu_window, true);
    simple_menu_layer_set_selected_index(my_menu_layer, 0 /* cancel snooze */, false);
}


void
note_config_provider (Window *window) 
{

    window_single_click_subscribe(BUTTON_ID_SELECT, note_select_click_handler);
}


void
my_config_provider (Window *window) 
{

    window_single_click_subscribe(BUTTON_ID_SELECT, my_select_click_handler);
}



/*
 * Only used if we were invoked from a background alarm
 */
void handle_note_tick(struct tm *tick_time, TimeUnits units_changed) {
//    int i;

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,	 
	    "note tick");

    now = time(0);
/*
 * Alarm every minute as long as the message window is being displayed
 */
    if (window_stack_contains_window(note_window) &&
        (now - last_vibes) > 30 /* seconds */) {
	vibes_enqueue_custom_pattern(alert_pulse);
        last_vibes = now;
    }

/*
 * If we need to write pin updates, do so here
 */
}


void
save_message (char *string, char **message) 
{
    int len;

    if (string && *string) {
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,
		"Save message: '%s'", string);
    }

    if (*message) {
	free(*message);
	*message = NULL;
    }
    if (string) {
	if (*string) {
            len = strlen(string)+1;
            if (len > MAX_MSG_LEN)
                len = MAX_MSG_LEN;
	    *message = malloc(strlen(string)+1);
	    if (*message) {
		strncpy(*message, string, len);
	    }
	}
    }
}


void
save_hour_mins(char *string, int *hour, int *min) 
{
    char *cp;
    char save_c;

    /* init */
    *hour = 0;
    *min = 0;

    /* find the ":" separator */
    for (cp = string; *cp && *cp != ':'; cp++) ;

    if (!*cp) {
	return;				/* error */
    }

    save_c = *cp;
    *cp = '\0';			/* null terminate hour */

    *hour = atoi(string);
    *min = atoi(++cp);
    *(--cp) = save_c;
}


void
subscribe_to_wakeups(AppLaunchReason reason) 
{
    int i;
    time_t now;
    struct tm *now_tick;

    now = time(0);
    now_tick = localtime(&now);

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "\n\n====== Subscribe to wakeups, now=%d", (int)now);

    if (reason != APP_LAUNCH_WAKEUP) {
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,
		"Cancelling all alarms");
	wakeup_cancel_all();		/* start over */
        for (i = 0 ; i < MAX_ALARMS-1; i++) {
            alarm_id[i] = 0;
            persist_write_int(alarmid_idx[i], (uint32_t)alarm_id[i]);
        }
    }

    /* Schedule alarms to set our alarm_time[] array */
    schedule_alarms(now, now_tick, reason != APP_LAUNCH_WAKEUP);
}


void
update_configuration (AppLaunchReason reason) 
{
    char string[MAX_MESS_LEN];
    int i;
    uint32_t val;

    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "\n\n======== Update configuration");

    for (i = 0 ; i < MAX_ALARMS ; i++) {
        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,
                "Alarm[%d] ===========", i);
	if (persist_exists(time_idx[i])) {
	    if (persist_read_string(time_idx[i],
				    string, sizeof(string)) > 0) {
		save_hour_mins(string,
			       &alarm_hours[i], &alarm_mins[i]);
	    }
            if (persist_exists(mess_idx[i])) {
                if (persist_read_string(mess_idx[i],
                                        string, sizeof(string)) > 0) {
                    save_message(string, &mess[i]);
                }
            }
        } else {
            alarm_hours[i] = 0;
            alarm_mins[i] = 0;
            save_message(NULL, &mess[i]); /* say there's no alarmnote here */
        }
        
	if (persist_exists(alarmid_idx[i])) {
            val = persist_read_int(alarmid_idx[i]);
            if (val) {
                alarm_id[i] = val;
	    }
	}

        alarm_time[i] = 0;              /* init */
	if (persist_exists(alarmtime_idx[i])) {
            val = persist_read_int(alarmtime_idx[i]);
            if (val) {
                alarm_time[i] = val;
	    }
	}

        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,
                "Alarm[%d] with idx=%u, saved as %d:%2d, "
                "alarm_time=%d",
                i, (uint)alarm_id[i], alarm_hours[i], alarm_mins[i],
                (int)alarm_time[i]);
    }
    dirty_pins = true;

#ifdef TESTING
    save_hour_mins("23:20", &alarm_hours[0], &alarm_mins[0]);
    save_message("Time to take your pills", &mess[0]);
#endif

    if (persist_exists(SNOOZEID)) {
	val = persist_read_int(SNOOZEID);
	if (val) {
	    snooze_id = (WakeupId)val;
	    
	    app_log(APP_LOG_LEVEL_WARNING,
		    __FILE__,
		    __LINE__,
		    "Snooze id read as %u",
		    (unsigned int)snooze_id);
	}
    }

    if (persist_exists(SNOOZEMSG)) {
	val = persist_read_int(SNOOZEMSG);
	if (val) {
	    snooze_msgid = (WakeupId)val;
	    
	    app_log(APP_LOG_LEVEL_WARNING,
		    __FILE__,
		    __LINE__,
		    "Snooze msg id read as %u",
		    (unsigned int)snooze_msgid);
	}
    }

    if (persist_exists(SNOOZETIME)) {
	val = persist_read_int(SNOOZETIME);
	if (val) {
	    snooze_time = (time_t)val;
	    
	    app_log(APP_LOG_LEVEL_WARNING,
		    __FILE__,
		    __LINE__,
		    "Snooze time read as %u",
		    (unsigned int)snooze_time);
	} 
   }

    if (persist_exists(APPSNOOZEID)) {
	val = persist_read_int(APPSNOOZEID);
	if (val) {
	    app_snooze_id = (AppTimer *)val;
	    
	    app_log(APP_LOG_LEVEL_WARNING,
		    __FILE__,
		    __LINE__,
		    "App Snooze id read as %u",
		    (unsigned int)app_snooze_id);
	}
    }

    if (persist_exists(FONTSIZE)) {
	val = persist_read_int(FONTSIZE);
	if (val) {
	    Font_size = (int)val;
	    
	    app_log(APP_LOG_LEVEL_WARNING,
		    __FILE__,
		    __LINE__,
		    "Font_size read as %u",
                    Font_size);
	}
    }

    /* Set a default, then set the right thing */
    note_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    for (i = 0 ; i < (int)(sizeof(Pebble_fonts) / sizeof(*Pebble_fonts)) ; i++) {
        if (Font_size == Pebble_fonts[i].font_size) {
            note_font = fonts_get_system_font(Pebble_fonts[i].pebble_font);
        }
    }

    if (persist_exists(DISABLE)) {
        bool val;
	val = persist_read_bool(DISABLE);
	if (val) {
	    alarms_disabled = (bool)val;
	    
	    app_log(APP_LOG_LEVEL_WARNING,
		    __FILE__,
		    __LINE__,
		    "Alarms disabled value is %u",
		    (unsigned int)alarms_disabled);
	} 
   }

    if (!alarms_disabled) {
        subscribe_to_wakeups(reason);
    } else {
        snprintf(text_message, sizeof(text_message),
                 "Alarms disabled\nUse \"back\" for menu");
        create_and_push_text_window(text_message, note_font);
    }
}


void handle_msg_received (DictionaryIterator *received, void *context)
{

    Tuple *tuple;
    int i;
    bool got_alarms=false;


    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "\n\n======== Received a msg");

/* Alarms */

    for (i = 0 ; i < MAX_ALARMS-1 ; i++) {
	tuple = dict_find(received, time_idx[i]);
        if (!tuple)
            continue;

        got_alarms=true;                /* found some alarm info */

	if (tuple->value->cstring &&
            *tuple->value->cstring) {
            app_log(APP_LOG_LEVEL_WARNING,
                    __FILE__,
                    __LINE__,
                    "Time for alarm %d is '%s'", i, tuple->value->cstring);
            save_hour_mins(tuple->value->cstring,
                           &alarm_hours[i], &alarm_mins[i]);
            persist_write_string(time_idx[i], tuple->value->cstring);
        
            tuple = dict_find(received, mess_idx[i]);
            if (tuple) {
                if (tuple->value->cstring &&
                    *tuple->value->cstring) {
//	    app_log(APP_LOG_LEVEL_WARNING,
//		    __FILE__,
//		    __LINE__,
//		    "Save message %d", i);
                    save_message(tuple->value->cstring, &mess[i]);
                    persist_write_string(mess_idx[i], mess[i]);
                } else {
                    app_log(APP_LOG_LEVEL_WARNING,
                            __FILE__,
                            __LINE__,
                            "Save NULL message %d", i);
                    save_message(NULL, &mess[i]);
                    persist_delete(mess_idx[i]);
                }
            }
        } else {
	    app_log(APP_LOG_LEVEL_WARNING,
		    __FILE__,
		    __LINE__,
		    "Save null for message %d", i);
            save_message(NULL, &mess[i]); /* say there's no alarmnote here */
            persist_delete(mess_idx[i]);
        }
    }
    
#ifdef notdef
    if (persist_exists(SNOOZEID)) {
	uint32_t val;
	
	val = persist_read_int(SNOOZEID);
	if (val) {
	    snooze_time = (WakeupId)val;
	    
	    app_log(APP_LOG_LEVEL_WARNING,
		    __FILE__,
		    __LINE__,
		    "Snooze id read as %u",
		    (unsigned int)snooze_time);
	}
    }

    if (persist_exists(SNOOZEMSG)) {
	uint32_t val;
	
	val = persist_read_int(SNOOZEMSG);
	if (val) {
	    snooze_msgid = (WakeupId)val;
	    
	    app_log(APP_LOG_LEVEL_WARNING,
		    __FILE__,
		    __LINE__,
		    "Snooze msg id read as %u",
		    (unsigned int)snooze_msgid);
	}
    }
#endif

    tuple = dict_find(received, 0);
    if (tuple) {
        got_connections = true;
        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,
                "Got a JS connection");
    }
    
    tuple = dict_find(received, FONTSIZE);
    if (tuple) {
        if (tuple->value->cstring) {
            Font_size = atoi(tuple->value->cstring);
        }
        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,
                "Got font size of %d", Font_size);
        persist_write_int(FONTSIZE, (int32_t)Font_size);
    }
    
    if (got_alarms) {
        update_configuration(APP_LAUNCH_USER);
        update_pins(0);
    } else if (dirty_pins && got_connections) {
        update_pins(0);
    }
}

void handle_msg_dropped (AppMessageResult reason, void *ctx)
{

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "Message dropped, reason code %d",
	    reason);
}


/*
 * Called from wakeup callback.
 */
void
handle_wakeup (WakeupId wakeup_id, int32_t cookie) 
{
    int i;
    bool snooze;
    int days;

    if (wakeup_id != 0) {
        wakeup_cancel(wakeup_id);
    }

    now = time(0);

    snooze = false;
    if ((int)cookie > 100) {
	snooze = true;
	i = (int)cookie - 100 - 1;
    } else {
	i = (int)cookie - 1;		/* our alarm number */
    }

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,	 
	    "Handling a wakeupid=%d, index=%d\n", (int)wakeup_id, i);

    last_wakeup_handled[i] = wakeup_id;

    /* Only look at first 7 alarms - alarm 8 is for snoozes */
    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,	 
	    "wakeup_id = %u, now = %u\n",
	    (unsigned int)wakeup_id,
	    (unsigned int)now);

    /*
     * Only reschedule for tomorrow, if this alarm is not a snooze
     */
    if (!snooze) {
        time_t alarm_tim;
        int32_t time_inc;
        struct tm *now_tick;

	/*
	 * The ID will only match for the official alarms,
	 * not for the snoozes.  This way, we guarantee we only
	 * reschedule the alarm once.
	 */
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,	 
		"Comparing Alarm_id[%d] at %u with %u\n",
		i, (unsigned int)alarm_time[i],
		(unsigned int)wakeup_id);

	/* reschedule for tomorrow */
        now_tick = localtime(&now);
        time_inc = ((alarm_hours[i] - now_tick->tm_hour) * HOURS) +
            ((alarm_mins[i] - now_tick->tm_min) * MINUTES) -
            now_tick->tm_sec;
        
        alarm_tim = now + time_inc;
        alarm_tim += 1 * DAYS;
        
        {
            time_t new_time;
            alarm_id[i] = schedule_my_wakeup(alarm_tim,
                                             i+1,
                                             "Error scheduling "
                                             "an alarm - %d", &new_time);
            alarm_tim = new_time;
        }
//	alarm_id[i] = wakeup_schedule(alarm_tim, i + 1, true);
        persist_write_int(alarmid_idx[i],
                          (uint32_t)alarm_id[i]);
        alarm_time[i] = alarm_tim;
        persist_write_int(alarmtime_idx[i],
                          (uint32_t)alarm_time[i]);
        days = (alarm_time[i] - now) / (1 * DAYS);
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,	 
		"Alarm_id[%d] rescheduled for %d day%c %u\n",
                days, (days > 1) ? 's' : ' ',
		i, (uint)alarm_tim);
        dirty_pins = true;
        update_pins(i);
    } else {
	/* Clear our saved snooze time */
	snooze_id = 0;
        snooze_time = 0;
        snooze_msgid = 0;
        app_snooze_id = NULL;
    }
    
    alarm_current = (int)i;

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "Sleep callback, alarm_current=%d", (int)alarm_current);

#ifdef notdef
    /*
     * If we have already displayed this alarm within the last
     * 3 minutes, don't display it again.
     */
    if (now - last_note_time < 3 * MINUTES &&
        last_note_alarm == alarm_current) {
        return;
    }
#endif

    display_pill_reminder(alarm_current);

    return;
}


#ifdef notdef
/*
 * Only called via app_timer_register for snooze events
 * "data" is set to the alarm_id index which caused the snooze
 */
void
snooze_callback (void *data) 
{
    int id = (uint)data - 100;
 
    handle_wakeup(alarm_id[id-1], (int32_t)data);
}
#endif


void
handle_bluetooth(bool connected) 
{
    static bool last_state=true;
    
    if (connected == false) {
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
		"Bluetooth lost");
	bluetooth_connected = false;
    } else {
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
		"Bluetooth back");
	bluetooth_connected = true;

        if (last_state == false) {
            update_pins(0);             /* it's back - update pins if needed */
        }
    }

    last_state = connected;
}




void handle_init(AppLaunchReason reason) {
    int i;
    int dict_size;
  
    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,	 
	    "Init\n");

    app_message_register_inbox_received(handle_msg_received);
    app_message_register_inbox_dropped(handle_msg_dropped);

    /* Calculate size of buffer needed */
    dict_size = dict_calc_buffer_size(26,
                                      sizeof(uint32_t), /* appkeyready */
                                      sizeof(uint32_t), /* fontsize */
                                      sizeof(uint32_t), /* a1 */
                                      sizeof(uint32_t), /* a2 */
                                      sizeof(uint32_t), /* a3 */
                                      sizeof(uint32_t), /* a4 */
                                      sizeof(uint32_t), /* a5 */
                                      sizeof(uint32_t), /* a6 */
                                      sizeof(uint32_t), /* aid0 */
                                      sizeof(uint32_t), /* aid1 */
                                      sizeof(uint32_t), /* aid2 */
                                      sizeof(uint32_t), /* aid3 */
                                      sizeof(uint32_t), /* aid4 */
                                      sizeof(uint32_t), /* aid5 */
                                      sizeof(uint32_t), /* aid6 */
                                      sizeof(uint32_t), /* aid7 */
                                      sizeof(uint32_t), /* snoozeid */
                                      sizeof(uint32_t), /* snoozetime */
                                      sizeof(uint32_t), /* appsnoozeid */
                                      MAX_MSG_LEN, MAX_MSG_LEN, MAX_MSG_LEN,
                                      MAX_MSG_LEN, MAX_MSG_LEN,
                                      MAX_MSG_LEN, MAX_MSG_LEN,
                                      MAX_MSG_LEN); /* character strings */
    
    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,	 
	    "dict_size = %d\n", dict_size);

    app_message_open(dict_size, dict_size);

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,	 
	    "reason = %u\n", (uint)reason);

    if (reason != APP_LAUNCH_WAKEUP) {
	
/*
 * Setup main window menu
 */
	my_menu_window = window_create();
	my_menu_layer = simple_menu_layer_create(layer_get_frame(window_get_root_layer(my_menu_window)),
                                                 my_menu_window, &my_menu_sections, num_my_menu_sections, NULL);
	layer_add_child(window_get_root_layer(my_menu_window), simple_menu_layer_get_layer(my_menu_layer));
	window_stack_push(my_menu_window, true);
    }

    tick_timer_service_subscribe(MINUTE_UNIT, handle_note_tick);
    
/*
 * Setup window for pill notifications
 */
    note_window = window_create();
#if defined(PBL_RECT)
    note_layer = text_layer_create(GRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT));
#elif defined(PBL_ROUND)
    Layer *window_layer = window_get_root_layer(note_window);
    GRect bounds = layer_get_bounds(window_layer);
    note_layer = text_layer_create(GRect(OFFSET,OFFSET,bounds.size.w-(OFFSET*2),bounds.size.h-(OFFSET*2)));
    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "set for round\n");
#endif
    text_layer_set_overflow_mode(note_layer, GTextOverflowModeWordWrap);
    layer_add_child(window_get_root_layer(note_window), text_layer_get_layer(note_layer));

//    if (reason == APP_LAUNCH_WAKEUP) {
//	window_stack_push(note_window, true);
//    }

/*
 * Setup click provider information for snooze window with menu
 */
    menu_window = window_create();
    window_set_click_config_provider(menu_window, (ClickConfigProvider) menu_config_provider);
    menu_layer = simple_menu_layer_create(layer_get_frame((const Layer *)note_layer), menu_window, &menu_sections, num_menu_sections, NULL);
    layer_add_child(window_get_root_layer(menu_window), simple_menu_layer_get_layer(menu_layer));
	
/*
 * Say to invoke our menu when pushing middle button on note window
 */
    window_set_click_config_provider(note_window, (ClickConfigProvider) note_config_provider);
	
    wakeup_service_subscribe(handle_wakeup);
    
    got_connections = false;

    alarms_disabled = false;

    for (i = 0 ; i < MAX_ALARMS ; i++) {
	alarm_id[i] = 0;		/* init. */
        alarm_time[i] = 0;
	alarm_missed[i] = 0;
	last_wakeup_handled[i] = 0;
    }

    bluetooth_connection_service_subscribe(handle_bluetooth);
    /* Update immediate status */
    if (bluetooth_connection_service_peek()) {
        bluetooth_connected = true;
    } else {
        bluetooth_connected = false;
    }

    update_configuration(reason);
}


void handle_deinit(void) {

    tick_timer_service_unsubscribe();

    bluetooth_connection_service_unsubscribe();
    app_message_deregister_callbacks();

    layer_destroy(my_window_layer);
    text_layer_destroy(note_layer);
    simple_menu_layer_destroy(menu_layer);
    simple_menu_layer_destroy(my_menu_layer);

    window_destroy(note_window);
    window_destroy(menu_window);
//    window_destroy(my_menu_window);

    if (notewin) {
	text_layer_destroy(notelayer);
	window_destroy(notewin);
    }

    if (alarm_window) {
	simple_menu_layer_destroy(alarm_win_layer);
	window_destroy(alarm_window);
    }
}




int main (void) {
    /* New for 2.0 */
    AppLaunchReason reason;
    WakeupId id = 0;
    int32_t cookie;

    reason = launch_reason();

    handle_init(reason);

    if (wakeup_get_launch_event(&id, &cookie)) {
        last_wakeup_cookie = cookie;
        last_wakeup_id = id;
        last_wakeup_time = time(0L);
	handle_wakeup(id, cookie);
    }
    
    app_event_loop();
    handle_deinit(); 
   /* end new */
}
