
/*
extern "C"
{
	#include "utils.h"
}
*/

#include <v8.h>
#include <nan.h>
#include <dbus/dbus.h>
#include "a2dp-codecs.h"

namespace a2dp {

	using namespace node;
	using namespace v8;
	using namespace std;

	const char *A2DP_SINK_ENDPOINT = "/MediaEndpoint/A2DPSink";
	const char *A2DP_SINK_UUID = "0000110b-0000-1000-8000-00805f9b34fb";
	const char *BLUEZ_SERVICE = "org.bluez";
	const char *BLUEZ_MEDIA_INTERFACE = "org.bluez.Media1";

	NAN_METHOD(SbcConfigFromByteArray) {
		unsigned char *buf = (unsigned char *) node::Buffer::Data(info[0].As<v8::Object>());
		a2dp_sbc_t *sbc_config;
		sbc_config = (a2dp_sbc_t *) buf;
		v8::Local<Object> obj = Nan::New<Object>();
		obj->Set(Nan::New("channelMode").ToLocalChecked(), Nan::New<Integer>(sbc_config->channel_mode));
		obj->Set(Nan::New("frequency").ToLocalChecked(), Nan::New<Integer>(sbc_config->frequency));
		obj->Set(Nan::New("allocationMethod").ToLocalChecked(), Nan::New<Integer>(sbc_config->allocation_method));
		obj->Set(Nan::New("subbands").ToLocalChecked(), Nan::New<Integer>(sbc_config->subbands));
		obj->Set(Nan::New("blockLength").ToLocalChecked(), Nan::New<Integer>(sbc_config->block_length));
		obj->Set(Nan::New("minBitpool").ToLocalChecked(), Nan::New<Integer>(sbc_config->min_bitpool));
		obj->Set(Nan::New("maxBitpool").ToLocalChecked(), Nan::New<Integer>(sbc_config->max_bitpool));
		info.GetReturnValue().Set(obj);
	}

	/*
	NAN_METHOD(ByteArrayFromSbcConfig) {

	}
	*/

	const char *signature_from_basic_type(int type) {
	    switch (type) {
	        case DBUS_TYPE_BOOLEAN: return DBUS_TYPE_BOOLEAN_AS_STRING;
	        case DBUS_TYPE_BYTE: return DBUS_TYPE_BYTE_AS_STRING;
	        case DBUS_TYPE_INT16: return DBUS_TYPE_INT16_AS_STRING;
	        case DBUS_TYPE_UINT16: return DBUS_TYPE_UINT16_AS_STRING;
	        case DBUS_TYPE_INT32: return DBUS_TYPE_INT32_AS_STRING;
	        case DBUS_TYPE_UINT32: return DBUS_TYPE_UINT32_AS_STRING;
	        case DBUS_TYPE_INT64: return DBUS_TYPE_INT64_AS_STRING;
	        case DBUS_TYPE_UINT64: return DBUS_TYPE_UINT64_AS_STRING;
	        case DBUS_TYPE_DOUBLE: return DBUS_TYPE_DOUBLE_AS_STRING;
	        case DBUS_TYPE_STRING: return DBUS_TYPE_STRING_AS_STRING;
	        case DBUS_TYPE_OBJECT_PATH: return DBUS_TYPE_OBJECT_PATH_AS_STRING;
	        case DBUS_TYPE_SIGNATURE: return DBUS_TYPE_SIGNATURE_AS_STRING;
	        default:
				//TODO: print log error here...
				return NULL;
	    }
	}

	unsigned basic_type_size(int type) {
	    switch (type) {
	        case DBUS_TYPE_BOOLEAN: return sizeof(dbus_bool_t);
	        case DBUS_TYPE_BYTE: return 1;
	        case DBUS_TYPE_INT16: return sizeof(dbus_int16_t);
	        case DBUS_TYPE_UINT16: return sizeof(dbus_uint16_t);
	        case DBUS_TYPE_INT32: return sizeof(dbus_int32_t);
	        case DBUS_TYPE_UINT32: return sizeof(dbus_uint32_t);
	        case DBUS_TYPE_INT64: return sizeof(dbus_int64_t);
	        case DBUS_TYPE_UINT64: return sizeof(dbus_uint64_t);
	        case DBUS_TYPE_DOUBLE: return sizeof(double);
	        case DBUS_TYPE_STRING:
	        case DBUS_TYPE_OBJECT_PATH:
	        case DBUS_TYPE_SIGNATURE: return sizeof(char*);
	        default:
	        	//TODO: print log error here...
	        	return 0;
	    }
	}

	void utils_dbus_append_basic_variant(DBusMessageIter *iter, int type, void *data) {
	    DBusMessageIter variant_iter;
	    dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, signature_from_basic_type(type), &variant_iter);
	    dbus_message_iter_append_basic(&variant_iter, type, data);
	    dbus_message_iter_close_container(iter, &variant_iter);
	}

	void utils_dbus_append_basic_variant_dict_entry(DBusMessageIter *dict_iter, const char *key, int type, void *data) {
	    DBusMessageIter dict_entry_iter;
	    dbus_message_iter_open_container(dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
	    dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &key);
	    utils_dbus_append_basic_variant(&dict_entry_iter, type, data);
	    dbus_message_iter_close_container(dict_iter, &dict_entry_iter);
	}

	void utils_dbus_append_basic_array(DBusMessageIter *iter, int item_type, const void *array, unsigned n) {
	    DBusMessageIter array_iter;
	    unsigned i;
	    unsigned item_size;
	    item_size = basic_type_size(item_type);
	    dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, signature_from_basic_type(item_type), &array_iter);
	    for (i = 0; i < n; ++i)
	        dbus_message_iter_append_basic(&array_iter, item_type, &((uint8_t*) array)[i * item_size]);
	    dbus_message_iter_close_container(iter, &array_iter);
	}

	void utils_dbus_append_basic_array_variant(DBusMessageIter *iter, int item_type, const void *array, unsigned n) {
	    DBusMessageIter variant_iter;
	    char array_signature[2];
	    sprintf(array_signature, "a%c", *signature_from_basic_type(item_type));
	    dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, array_signature, &variant_iter);
	    utils_dbus_append_basic_array(&variant_iter, item_type, array, n);
	    dbus_message_iter_close_container(iter, &variant_iter);
	}

	void utils_dbus_append_basic_array_variant_dict_entry(
	        DBusMessageIter *dict_iter,
	        const char *key,
	        int item_type,
	        const void *array,
	        unsigned n) {
	    DBusMessageIter dict_entry_iter;
	    dbus_message_iter_open_container(dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
	    dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &key);
	    utils_dbus_append_basic_array_variant(&dict_entry_iter, item_type, array, n);
	    dbus_message_iter_close_container(dict_iter, &dict_entry_iter);
	}

	void send_and_add_to_pending(
			DBusConnection *connection, DBusMessage *m,
			DBusPendingCallNotifyFunction func, void *call_data) {

	    DBusPendingCall *call;
	    dbus_connection_send_with_reply(connection, m, &call, -1);
	    dbus_pending_call_set_notify(call, func, call_data, NULL);
	}

	const char *utils_dbus_get_error_message(DBusMessage *m) {
	    const char *message;

	    if (dbus_message_get_signature(m)[0] != 's')
	        return "<no explanation>";

	    dbus_message_get_args(m, NULL, DBUS_TYPE_STRING, &message, DBUS_TYPE_INVALID);

	    return message;
	}

	void register_endpoint_reply(DBusPendingCall *pending, void *endpoint) {
		printf("register_endpoint_reply -> endpoint: %s \n", (char*) endpoint);
		DBusMessage *r;
		r = dbus_pending_call_steal_reply(pending);
	    if (dbus_message_get_type(r) == DBUS_MESSAGE_TYPE_ERROR) {
	        printf("%s.RegisterEndpoint() failed: %s: %s", BLUEZ_MEDIA_INTERFACE, dbus_message_get_error_name(r),
	                     utils_dbus_get_error_message(r));
	        goto finish;
	    }
		finish:
			dbus_message_unref(r);
			free(endpoint);
	}

	void register_endpoint(DBusConnection *connection, const char *adapter_path, const char *endpoint, const char *uuid) {

	    DBusMessage *m;
	    DBusMessageIter i, d;
	    uint8_t codec = 0;

	    m = dbus_message_new_method_call(BLUEZ_SERVICE, adapter_path, BLUEZ_MEDIA_INTERFACE, "RegisterEndpoint");
	    dbus_message_iter_init_append(m, &i);
	    dbus_message_iter_append_basic(&i, DBUS_TYPE_OBJECT_PATH, &endpoint);
	    dbus_message_iter_open_container(&i, DBUS_TYPE_ARRAY, "{sv}", &d);
	    utils_dbus_append_basic_variant_dict_entry(&d, "UUID", DBUS_TYPE_STRING, &uuid);
	    utils_dbus_append_basic_variant_dict_entry(&d, "Codec", DBUS_TYPE_BYTE, &codec);

        a2dp_sbc_t capabilities;
        capabilities.channel_mode = SBC_CHANNEL_MODE_MONO | SBC_CHANNEL_MODE_DUAL_CHANNEL | SBC_CHANNEL_MODE_STEREO |
                                    SBC_CHANNEL_MODE_JOINT_STEREO;
        capabilities.frequency = SBC_SAMPLING_FREQ_16000 | SBC_SAMPLING_FREQ_32000 | SBC_SAMPLING_FREQ_44100 |
                                 SBC_SAMPLING_FREQ_48000;
        capabilities.allocation_method = SBC_ALLOCATION_SNR | SBC_ALLOCATION_LOUDNESS;
        capabilities.subbands = SBC_SUBBANDS_4 | SBC_SUBBANDS_8;
        capabilities.block_length = SBC_BLOCK_LENGTH_4 | SBC_BLOCK_LENGTH_8 | SBC_BLOCK_LENGTH_12 | SBC_BLOCK_LENGTH_16;
        capabilities.min_bitpool = MIN_BITPOOL;
        capabilities.max_bitpool = MAX_BITPOOL;

        utils_dbus_append_basic_array_variant_dict_entry(&d, "Capabilities", DBUS_TYPE_BYTE, &capabilities, sizeof(capabilities));
        dbus_message_iter_close_container(&i, &d);

        char *call_data = (char *) malloc(strlen(endpoint)+1);
        strcpy(call_data, endpoint);
        send_and_add_to_pending(connection, m, register_endpoint_reply, call_data);
	}

	DBusHandlerResult endpoint_handler(DBusConnection *c, DBusMessage *m, void *userdata) {


	    return DBUS_HANDLER_RESULT_HANDLED;
	}

	/*
	void endpoint_sink_init() {

		DBusObjectPathVTable vtable_endpoint;
		vtable_endpoint.message_function = endpoint_handler;

        pa_assert_se(dbus_connection_register_object_path(pa_dbus_connection_get(y->connection), A2DP_SOURCE_ENDPOINT,
                                                          &vtable_endpoint, y));

	    switch(profile) {

	        case PA_BLUETOOTH_PROFILE_A2DP_SINK:
	            pa_assert_se(dbus_connection_register_object_path(
	            		pa_dbus_connection_get(y->connection),
						A2DP_SOURCE_ENDPOINT,
	                    &vtable_endpoint, y));
	            break;

	        case PA_BLUETOOTH_PROFILE_A2DP_SOURCE:
	            pa_assert_se(dbus_connection_register_object_path(pa_dbus_connection_get(y->connection), A2DP_SINK_ENDPOINT,
	                                                              &vtable_endpoint, y));
	            break;

	        default:
	            pa_assert_not_reached();
	            break;

	    }
	}
	*/

	static void init(Local<Object> exports) {

		Nan::SetMethod(exports, "sbcConfigFromByteArray", SbcConfigFromByteArray);

	}

	NODE_MODULE(a2dp, init);

}
