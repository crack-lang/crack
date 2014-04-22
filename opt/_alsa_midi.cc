#include <alsa/asoundlib.h>
snd_seq_event_t *snd_seq_event_alloc() {
    snd_seq_event_t *e = new snd_seq_event_t;
    snd_seq_ev_clear(e);    return e; }
void snd_seq_event_setSource(snd_seq_event_t *event, int port) {
    event->source.port = port; }
void snd_seq_event_setSubs(snd_seq_event_t *event) {
    event->dest.client = SND_SEQ_ADDRESS_SUBSCRIBERS;
    event->dest.port = SND_SEQ_ADDRESS_UNKNOWN; }
void snd_seq_event_setDirect(snd_seq_event_t *event) {
    event->queue = SND_SEQ_QUEUE_DIRECT; }
void snd_seq_event_setNoteOn(snd_seq_event_t *event,                             int channel,                             int note,                             int velocity) {    snd_seq_ev_set_noteon(event, channel, note, velocity); }void snd_seq_event_setNoteOff(snd_seq_event_t *event,                              int channel,                              int note,                              int velocity) {    snd_seq_ev_set_noteoff(event, channel, note, velocity); }void snd_seq_event_scheduleTick(snd_seq_event_t *event, int queue,                                 int relative,                                int time) {    snd_seq_ev_schedule_tick(event, queue, relative, time); }void SndSeqQueue_setTempo(snd_seq_t *seqp, int queueId,                           int tempo, int ppq) {    snd_seq_queue_tempo_t *t;    snd_seq_queue_tempo_alloca(&t);     snd_seq_queue_tempo_set_tempo(t, tempo);    snd_seq_queue_tempo_set_ppq(t, ppq);    snd_seq_set_queue_tempo(seqp, queueId, t); }void SndSeqQueue_start(snd_seq_t *seqp, int queueId, snd_seq_event_t *event) {    snd_seq_start_queue(seqp, queueId, event); }void SndSeqQueue_stop(snd_seq_t *seqp, int queueId, snd_seq_event_t *event) {    snd_seq_stop_queue(seqp, queueId, event); }
    void snd_seq_event_setController(snd_seq_event_t *event,
                                     int channel,
                                     int cc,
                                     int val
                                     ) {
        snd_seq_ev_set_controller(event, channel, cc, val);
    }
    snd_seq_ev_note_t *snd_seq_event_getNote(snd_seq_event_t *event) {
        return &event->data.note;
    }
    snd_seq_ev_ctrl_t *snd_seq_event_getControl(snd_seq_event_t *event) {
        return &event->data.control;
    }
    snd_seq_ev_ext_t *snd_seq_event_getExt(snd_seq_event_t *event) {
        return &event->data.ext;
    }
typedef int Undef;


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__alsa_midi_rinit() {
    return;
}

extern "C"
void crack_ext__alsa_midi_cinit(crack::ext::Module *mod) {
    crack::ext::Func *f;
    crack::ext::Type *type_Class = mod->getClassType();
    crack::ext::Type *type_void = mod->getVoidType();
    crack::ext::Type *type_voidptr = mod->getVoidptrType();
    crack::ext::Type *type_bool = mod->getBoolType();
    crack::ext::Type *type_byteptr = mod->getByteptrType();
    crack::ext::Type *type_byte = mod->getByteType();
    crack::ext::Type *type_int16 = mod->getInt16Type();
    crack::ext::Type *type_int32 = mod->getInt32Type();
    crack::ext::Type *type_int64 = mod->getInt64Type();
    crack::ext::Type *type_uint16 = mod->getUint16Type();
    crack::ext::Type *type_uint32 = mod->getUint32Type();
    crack::ext::Type *type_uint64 = mod->getUint64Type();
    crack::ext::Type *type_int = mod->getIntType();
    crack::ext::Type *type_uint = mod->getUintType();
    crack::ext::Type *type_intz = mod->getIntzType();
    crack::ext::Type *type_uintz = mod->getUintzType();
    crack::ext::Type *type_float32 = mod->getFloat32Type();
    crack::ext::Type *type_float64 = mod->getFloat64Type();
    crack::ext::Type *type_float = mod->getFloatType();

    crack::ext::Type *type_snd_seq_t = mod->addType("snd_seq_t", sizeof(Undef));
    type_snd_seq_t->finish();


    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_psnd__seq__t_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_snd_seq_t;
        array_psnd__seq__t_q = array->getSpecialization(params);
    }

    crack::ext::Type *type_snd_seq_ev_note_t = mod->addType("snd_seq_ev_note_t", sizeof(snd_seq_ev_note_t));
        type_snd_seq_ev_note_t->addInstVar(type_byte, "channel",
                                CRACK_OFFSET(snd_seq_ev_note_t, channel));
        type_snd_seq_ev_note_t->addInstVar(type_byte, "note",
                                CRACK_OFFSET(snd_seq_ev_note_t, note));
        type_snd_seq_ev_note_t->addInstVar(type_byte, "velocity",
                                CRACK_OFFSET(snd_seq_ev_note_t, velocity));
        type_snd_seq_ev_note_t->addInstVar(type_byte, "off_velocity",
                                CRACK_OFFSET(snd_seq_ev_note_t, off_velocity));
        type_snd_seq_ev_note_t->addInstVar(type_uint, "duration",
                                CRACK_OFFSET(snd_seq_ev_note_t, duration));
    type_snd_seq_ev_note_t->finish();


    crack::ext::Type *type_snd_seq_ev_ctrl_t = mod->addType("snd_seq_ev_ctrl_t", sizeof(snd_seq_ev_ctrl_t));
        type_snd_seq_ev_ctrl_t->addInstVar(type_byte, "channel",
                                CRACK_OFFSET(snd_seq_ev_ctrl_t, channel));
        type_snd_seq_ev_ctrl_t->addInstVar(type_uint, "param",
                                CRACK_OFFSET(snd_seq_ev_ctrl_t, param));
        type_snd_seq_ev_ctrl_t->addInstVar(type_int, "value",
                                CRACK_OFFSET(snd_seq_ev_ctrl_t, value));
    type_snd_seq_ev_ctrl_t->finish();


    crack::ext::Type *type_snd_seq_ev_ext_t = mod->addType("snd_seq_ev_ext_t", sizeof(snd_seq_ev_ext_t));
        type_snd_seq_ev_ext_t->addInstVar(type_uint, "len",
                                CRACK_OFFSET(snd_seq_ev_ext_t, len));
        type_snd_seq_ev_ext_t->addInstVar(type_byteptr, "ptr",
                                CRACK_OFFSET(snd_seq_ev_ext_t, ptr));
    type_snd_seq_ev_ext_t->finish();


    crack::ext::Type *type_snd_seq_event_t = mod->addType("snd_seq_event_t", sizeof(snd_seq_event_t));
        type_snd_seq_event_t->addInstVar(type_byte, "type",
                                CRACK_OFFSET(snd_seq_event_t, type));
        type_snd_seq_event_t->addInstVar(type_byte, "flags",
                                CRACK_OFFSET(snd_seq_event_t, flags));
        type_snd_seq_event_t->addInstVar(type_byte, "tag",
                                CRACK_OFFSET(snd_seq_event_t, tag));
        type_snd_seq_event_t->addInstVar(type_byte, "queue",
                                CRACK_OFFSET(snd_seq_event_t, queue));


    f = type_snd_seq_event_t->addMethod(
        type_void, 
        "setSource",
        (void *)snd_seq_event_setSource
    );
    f->addArg(type_int, 
              "port"
              );


    f = type_snd_seq_event_t->addMethod(
        type_void, 
        "setSubs",
        (void *)snd_seq_event_setSubs
    );


    f = type_snd_seq_event_t->addMethod(
        type_void, 
        "setDirect",
        (void *)snd_seq_event_setDirect
    );


    f = type_snd_seq_event_t->addMethod(
        type_void, 
        "setNoteOn",
        (void *)snd_seq_event_setNoteOn
    );
    f->addArg(type_int, 
              "channel"
              );
    f->addArg(type_int, 
              "note"
              );
    f->addArg(type_int, 
              "velocity"
              );


    f = type_snd_seq_event_t->addMethod(
        type_void, 
        "setNoteOff",
        (void *)snd_seq_event_setNoteOff
    );
    f->addArg(type_int, 
              "channel"
              );
    f->addArg(type_int, 
              "note"
              );
    f->addArg(type_int, 
              "velocity"
              );


    f = type_snd_seq_event_t->addMethod(
        type_void, 
        "scheduleTick",
        (void *)snd_seq_event_scheduleTick
    );
    f->addArg(type_int, 
              "queue"
              );
    f->addArg(type_int, 
              "relative"
              );
    f->addArg(type_int, 
              "time"
              );


    f = type_snd_seq_event_t->addMethod(
        type_void, 
        "setController",
        (void *)snd_seq_event_setController
    );
    f->addArg(type_int, 
              "channel"
              );
    f->addArg(type_int, 
              "cc"
              );
    f->addArg(type_int, 
              "val"
              );


    f = type_snd_seq_event_t->addMethod(
        type_snd_seq_ev_note_t, 
        "getNote",
        (void *)snd_seq_event_getNote
    );


    f = type_snd_seq_event_t->addMethod(
        type_snd_seq_ev_ctrl_t, 
        "getControl",
        (void *)snd_seq_event_getControl
    );


    f = type_snd_seq_event_t->addMethod(
        type_snd_seq_ev_ext_t, 
        "getExt",
        (void *)snd_seq_event_getExt
    );

    type_snd_seq_event_t->finish();


    crack::ext::Type *array_psnd__seq__event__t_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_snd_seq_event_t;
        array_psnd__seq__event__t_q = array->getSpecialization(params);
    }
    f = mod->addFunc(type_int, "snd_seq_open",
                     (void *)snd_seq_open
                     );
       f->addArg(array_psnd__seq__t_q, "seqp");
       f->addArg(type_byteptr, "name");
       f->addArg(type_int, "streams");
       f->addArg(type_int, "mode");

    f = mod->addFunc(type_int, "snd_seq_set_client_name",
                     (void *)snd_seq_set_client_name
                     );
       f->addArg(type_snd_seq_t, "seqp");
       f->addArg(type_byteptr, "name");

    f = mod->addFunc(type_int, "snd_seq_create_simple_port",
                     (void *)snd_seq_create_simple_port
                     );
       f->addArg(type_snd_seq_t, "handle");
       f->addArg(type_byteptr, "name");
       f->addArg(type_int, "caps");
       f->addArg(type_int, "type");

    f = mod->addFunc(type_int, "snd_seq_connect_to",
                     (void *)snd_seq_connect_to
                     );
       f->addArg(type_snd_seq_t, "seq");
       f->addArg(type_int, "myport");
       f->addArg(type_int, "dest_client");
       f->addArg(type_int, "dest_port");

    f = mod->addFunc(type_int, "snd_seq_event_output",
                     (void *)snd_seq_event_output
                     );
       f->addArg(type_snd_seq_t, "seqp");
       f->addArg(type_snd_seq_event_t, "event");

    f = mod->addFunc(type_int, "snd_seq_drain_output",
                     (void *)snd_seq_drain_output
                     );
       f->addArg(type_snd_seq_t, "seqp");

    f = mod->addFunc(type_int, "snd_seq_event_input",
                     (void *)snd_seq_event_input
                     );
       f->addArg(type_snd_seq_t, "seqp");
       f->addArg(array_psnd__seq__event__t_q, "event");

    f = mod->addFunc(type_int, "snd_seq_event_input_pending",
                     (void *)snd_seq_event_input_pending
                     );
       f->addArg(type_snd_seq_t, "seqp");
       f->addArg(type_int, "fetch_sequencer");

    f = mod->addFunc(type_int, "snd_seq_alloc_named_queue",
                     (void *)snd_seq_alloc_named_queue
                     );
       f->addArg(type_snd_seq_t, "seqp");
       f->addArg(type_byteptr, "name");

    f = mod->addFunc(type_int, "SndSeqQueue_setTempo",
                     (void *)SndSeqQueue_setTempo
                     );
       f->addArg(type_snd_seq_t, "seqp");
       f->addArg(type_int, "queueId");
       f->addArg(type_int, "tempo");
       f->addArg(type_int, "ppq");

    f = mod->addFunc(type_void, "SndSeqQueue_start",
                     (void *)SndSeqQueue_start
                     );
       f->addArg(type_snd_seq_t, "seqp");
       f->addArg(type_int, "queueId");
       f->addArg(type_snd_seq_event_t, "event");

    f = mod->addFunc(type_void, "SndSeqQueue_stop",
                     (void *)SndSeqQueue_stop
                     );
       f->addArg(type_snd_seq_t, "seqp");
       f->addArg(type_int, "queueId");
       f->addArg(type_snd_seq_event_t, "event");


    mod->addConstant(type_int, "SND_SEQ_OPEN_INPUT",
                     static_cast<int>(SND_SEQ_OPEN_INPUT)
                     );

    mod->addConstant(type_int, "SND_SEQ_OPEN_OUTPUT",
                     static_cast<int>(SND_SEQ_OPEN_OUTPUT)
                     );

    mod->addConstant(type_int, "SND_SEQ_OPEN_DUPLEX",
                     static_cast<int>(SND_SEQ_OPEN_DUPLEX)
                     );

    mod->addConstant(type_int, "SND_SEQ_PORT_CAP_WRITE",
                     static_cast<int>(SND_SEQ_PORT_CAP_WRITE)
                     );

    mod->addConstant(type_int, "SND_SEQ_PORT_CAP_SUBS_WRITE",
                     static_cast<int>(SND_SEQ_PORT_CAP_SUBS_WRITE)
                     );

    mod->addConstant(type_int, "SND_SEQ_PORT_CAP_READ",
                     static_cast<int>(SND_SEQ_PORT_CAP_READ)
                     );

    mod->addConstant(type_int, "SND_SEQ_PORT_CAP_SUBS_READ",
                     static_cast<int>(SND_SEQ_PORT_CAP_SUBS_READ)
                     );

    mod->addConstant(type_int, "SND_SEQ_PORT_TYPE_MIDI_GENERIC",
                     static_cast<int>(SND_SEQ_PORT_TYPE_MIDI_GENERIC)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_SYSTEM",
                     static_cast<int>(SND_SEQ_EVENT_SYSTEM)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_RESULT",
                     static_cast<int>(SND_SEQ_EVENT_RESULT)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_NOTE",
                     static_cast<int>(SND_SEQ_EVENT_NOTE)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_NOTEON",
                     static_cast<int>(SND_SEQ_EVENT_NOTEON)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_NOTEOFF",
                     static_cast<int>(SND_SEQ_EVENT_NOTEOFF)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_KEYPRESS",
                     static_cast<int>(SND_SEQ_EVENT_KEYPRESS)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_CONTROLLER",
                     static_cast<int>(SND_SEQ_EVENT_CONTROLLER)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_PGMCHANGE",
                     static_cast<int>(SND_SEQ_EVENT_PGMCHANGE)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_CHANPRESS",
                     static_cast<int>(SND_SEQ_EVENT_CHANPRESS)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_PITCHBEND",
                     static_cast<int>(SND_SEQ_EVENT_PITCHBEND)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_CONTROL14",
                     static_cast<int>(SND_SEQ_EVENT_CONTROL14)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_NONREGPARAM",
                     static_cast<int>(SND_SEQ_EVENT_NONREGPARAM)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_REGPARAM",
                     static_cast<int>(SND_SEQ_EVENT_REGPARAM)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_SONGPOS",
                     static_cast<int>(SND_SEQ_EVENT_SONGPOS)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_SONGSEL",
                     static_cast<int>(SND_SEQ_EVENT_SONGSEL)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_QFRAME",
                     static_cast<int>(SND_SEQ_EVENT_QFRAME)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_TIMESIGN",
                     static_cast<int>(SND_SEQ_EVENT_TIMESIGN)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_KEYSIGN",
                     static_cast<int>(SND_SEQ_EVENT_KEYSIGN)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_START",
                     static_cast<int>(SND_SEQ_EVENT_START)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_CONTINUE",
                     static_cast<int>(SND_SEQ_EVENT_CONTINUE)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_STOP",
                     static_cast<int>(SND_SEQ_EVENT_STOP)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_SETPOS_TICK",
                     static_cast<int>(SND_SEQ_EVENT_SETPOS_TICK)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_SETPOS_TIME",
                     static_cast<int>(SND_SEQ_EVENT_SETPOS_TIME)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_TEMPO",
                     static_cast<int>(SND_SEQ_EVENT_TEMPO)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_CLOCK",
                     static_cast<int>(SND_SEQ_EVENT_CLOCK)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_TICK",
                     static_cast<int>(SND_SEQ_EVENT_TICK)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_QUEUE_SKEW",
                     static_cast<int>(SND_SEQ_EVENT_QUEUE_SKEW)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_SYNC_POS",
                     static_cast<int>(SND_SEQ_EVENT_SYNC_POS)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_TUNE_REQUEST",
                     static_cast<int>(SND_SEQ_EVENT_TUNE_REQUEST)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_RESET",
                     static_cast<int>(SND_SEQ_EVENT_RESET)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_SENSING",
                     static_cast<int>(SND_SEQ_EVENT_SENSING)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_ECHO",
                     static_cast<int>(SND_SEQ_EVENT_ECHO)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_OSS",
                     static_cast<int>(SND_SEQ_EVENT_OSS)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_CLIENT_START",
                     static_cast<int>(SND_SEQ_EVENT_CLIENT_START)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_CLIENT_EXIT",
                     static_cast<int>(SND_SEQ_EVENT_CLIENT_EXIT)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_CLIENT_CHANGE",
                     static_cast<int>(SND_SEQ_EVENT_CLIENT_CHANGE)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_PORT_START",
                     static_cast<int>(SND_SEQ_EVENT_PORT_START)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_PORT_EXIT",
                     static_cast<int>(SND_SEQ_EVENT_PORT_EXIT)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_PORT_CHANGE",
                     static_cast<int>(SND_SEQ_EVENT_PORT_CHANGE)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_PORT_SUBSCRIBED",
                     static_cast<int>(SND_SEQ_EVENT_PORT_SUBSCRIBED)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_PORT_UNSUBSCRIBED",
                     static_cast<int>(SND_SEQ_EVENT_PORT_UNSUBSCRIBED)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_USR0",
                     static_cast<int>(SND_SEQ_EVENT_USR0)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_USR1",
                     static_cast<int>(SND_SEQ_EVENT_USR1)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_USR2",
                     static_cast<int>(SND_SEQ_EVENT_USR2)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_USR3",
                     static_cast<int>(SND_SEQ_EVENT_USR3)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_USR4",
                     static_cast<int>(SND_SEQ_EVENT_USR4)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_USR5",
                     static_cast<int>(SND_SEQ_EVENT_USR5)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_USR6",
                     static_cast<int>(SND_SEQ_EVENT_USR6)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_USR7",
                     static_cast<int>(SND_SEQ_EVENT_USR7)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_USR8",
                     static_cast<int>(SND_SEQ_EVENT_USR8)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_USR9",
                     static_cast<int>(SND_SEQ_EVENT_USR9)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_SYSEX",
                     static_cast<int>(SND_SEQ_EVENT_SYSEX)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_BOUNCE",
                     static_cast<int>(SND_SEQ_EVENT_BOUNCE)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_USR_VAR0",
                     static_cast<int>(SND_SEQ_EVENT_USR_VAR0)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_USR_VAR1",
                     static_cast<int>(SND_SEQ_EVENT_USR_VAR1)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_USR_VAR2",
                     static_cast<int>(SND_SEQ_EVENT_USR_VAR2)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_USR_VAR3",
                     static_cast<int>(SND_SEQ_EVENT_USR_VAR3)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_USR_VAR4",
                     static_cast<int>(SND_SEQ_EVENT_USR_VAR4)
                     );

    mod->addConstant(type_int, "SND_SEQ_EVENT_NONE",
                     static_cast<int>(SND_SEQ_EVENT_NONE)
                     );
}
