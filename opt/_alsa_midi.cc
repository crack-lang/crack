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
void snd_seq_event_setNoteOn(snd_seq_event_t *event,                             int channel,                             int note,                             int velocity) {    snd_seq_ev_set_noteon(event, channel, note, velocity); }void snd_seq_event_setNoteOff(snd_seq_event_t *event,                              int channel,                              int note,                              int velocity) {    snd_seq_ev_set_noteoff(event, channel, note, velocity); }void snd_seq_event_scheduleTick(snd_seq_event_t *event, int queue,                                 int relative,                                int time) {    snd_seq_ev_schedule_tick(event, queue, relative, time); }void SndSeqQueue_setTempo(snd_seq_t *seqp, int queueId,                           int tempo, int ppq) {    snd_seq_queue_tempo_t *t;    snd_seq_queue_tempo_alloca(&t);     snd_seq_queue_tempo_set_tempo(t, tempo);    snd_seq_queue_tempo_set_ppq(t, ppq);    snd_seq_set_queue_tempo(seqp, queueId, t); }void SndSeqQueue_start(snd_seq_t *seqp, int queueId, snd_seq_event_t *event) {    snd_seq_start_queue(seqp, queueId, event); }void SndSeqQueue_stop(snd_seq_t *seqp, int queueId, snd_seq_event_t *event) {    snd_seq_stop_queue(seqp, queueId, event); }typedef int Undef;


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
    crack::ext::Type *type_int32 = mod->getInt32Type();
    crack::ext::Type *type_int64 = mod->getInt64Type();
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

    crack::ext::Type *type_snd_seq_event_t = mod->addType("snd_seq_event_t", sizeof(snd_seq_event_t));
        f = type_snd_seq_event_t->addConstructor("init",
                        (void *)snd_seq_event_alloc
                );

        f = type_snd_seq_event_t->addMethod(type_void, "setSource",
                        (void *)snd_seq_event_setSource
                );
            f->addArg(type_int, "port");

        f = type_snd_seq_event_t->addMethod(type_void, "setSubs",
                        (void *)snd_seq_event_setSubs
                );

        f = type_snd_seq_event_t->addMethod(type_void, "setDirect",
                        (void *)snd_seq_event_setDirect
                );

        f = type_snd_seq_event_t->addMethod(type_void, "setNoteOn",
                        (void *)snd_seq_event_setNoteOn
                );
            f->addArg(type_int, "channel");
            f->addArg(type_int, "note");
            f->addArg(type_int, "velocity");

        f = type_snd_seq_event_t->addMethod(type_void, "setNoteOff",
                        (void *)snd_seq_event_setNoteOff
                );
            f->addArg(type_int, "channel");
            f->addArg(type_int, "note");
            f->addArg(type_int, "velocity");

        f = type_snd_seq_event_t->addMethod(type_void, "scheduleTick",
                        (void *)snd_seq_event_scheduleTick
                );
            f->addArg(type_int, "queue");
            f->addArg(type_int, "relative");
            f->addArg(type_int, "time");

    type_snd_seq_event_t->finish();

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
}
