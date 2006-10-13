/*
 * <krb5/preauth_plugin.h>
 *
 * Copyright (c) 2006 Red Hat, Inc.
 * Portions copyright (c) 2006 Massachusetts Institute of Technology
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Red Hat, Inc., nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Preauthentication plugin definitions for Kerberos 5.
 */

#ifndef KRB5_PREAUTH_PLUGIN_H_INCLUDED
#define KRB5_PREAUTH_PLUGIN_H_INCLUDED
#include <krb5/krb5.h>

/*
 * While arguments of these types are passed-in, for the most part a preauth
 * module can treat them as opaque.  If we need keying data, we can ask for
 * it directly.
 */
struct _krb5_db_entry_new;
struct _krb5_key_data;

/*
 * Preauth mechanism property flags, unified from previous definitions in the
 * KDC and libkrb5 sources.
 */

/* Provides a real answer which we can send back to the KDC (client-only).  The
 * client assumes that one real answer will be enough. */
#define PA_REAL		0x00000001

/* Doesn't provide a real answer, but must be given a chance to run before any
 * REAL mechanism callbacks (client-only). */
#define PA_INFO		0x00000002

/* Causes the KDC to include this mechanism in a list of supported preauth
 * types if the user's DB entry flags the user as requiring hardware-based
 * preauthentication (server-only). */
#define PA_HARDWARE	0x00000004

/* Causes the KDC to include this mechanism in a list of supported preauth
 * types if the user's DB entry flags the user as requiring preauthentication,
 * and to fail preauthentication if we can't verify the client data.  The
 * flipside of PA_SUFFICIENT (server-only). */
#define PA_REQUIRED	0x00000008

/* Causes the KDC to include this mechanism in a list of supported preauth
 * types if the user's DB entry flags the user as requiring preauthentication,
 * and to mark preauthentication as successful if we can verify the client
 * data.  The flipside of PA_REQUIRED (server-only). */
#define PA_SUFFICIENT	0x00000010

/* Marks this preauthentication mechanism as one which changes the key which is
 * used for encrypting the response to the client.  Modules which have this
 * flag have their server_return_proc called before modules which do not, and
 * are passed over if a previously-called module has modified the encrypting
 * key (server-only). */
#define PA_REPLACES_KEY	0x00000020

/* Causes the KDC to check with this preauthentication module even if the
 * client has no entry in the realm database.  If the module returns a success
 * code, continue processing and assume that its return_padata callback will
 * supply us with a key for encrypting the AS reply (server-only). */
/* #define PA_VIRTUAL	(0x00000040 | PA_REPLACES_KEY) */

/* Not really a padata type, so don't include it in any list of preauth types
 * which gets sent over the wire. */
#define PA_PSEUDO	0x00000080

/*
 * A server module's callback functions are allowed to request specific types
 * of information about the given client or server record or request, even
 * though the database records themselves are opaque to the module.
 */
enum krb5plugin_preauth_entry_request_type {
    /* The returned krb5_data item holds a DER-encoded X.509 certificate. */
    krb5plugin_preauth_entry_request_certificate = 1,
    /* The returned krb5_data_item holds a krb5_deltat. */
    krb5plugin_preauth_entry_max_time_skew = 2,
    /* The returned krb5_data_item holds an array of krb5_keyblock structures,
     * terminated by an entry with key type = 0.
     * Each keyblock should have its contents freed in turn, and then the data
     * item itself should be freed. */
    krb5plugin_preauth_keys = 3,
    /* The returned krb5_data_item holds the request structure, re-encoded
     * using DER.  Unless the client implementation is the same as the server
     * implementation, there's a good chance that the result will not match
     * what the client sent, so don't go creating any fatal errors if it
     * doesn't match up. */
    krb5plugin_preauth_request_body = 4
};
typedef krb5_error_code
(*preauth_get_entry_data_proc)(krb5_context,
			       krb5_kdc_req *,
			       struct _krb5_db_entry_new *,
			       krb5_int32 request_type,
			       krb5_data **);

/*
 * A callback which will obtain the user's long-term AS key by prompting the
 * user for the password, then salting it properly, and so on.  For the moment,
 * it's identical to the get_as_key callback used inside of libkrb5, but we
 * define a new typedef here instead of making the existing one public to
 * isolate ourselves from potential future changes.
 */
typedef krb5_error_code
(*preauth_get_as_key_proc)(krb5_context,
			   krb5_principal,
			   krb5_enctype,
			   krb5_prompter_fct,
			   void *prompter_data,
			   krb5_data *salt,
			   krb5_data *s2kparams,
			   krb5_keyblock *as_key,
			   void *gak_data);

/*
 * The function table / structure which a preauth client module must export as
 * "preauthentication_client_0".  If the interfaces work correctly, future
 * versions of the table will add either more callbacks or more arguments to
 * callbacks, and in both cases we'll be able to wrap the v0 functions.
 */
typedef struct krb5plugin_preauth_client_ftable_v0 {
    /* Not-usually-visible name. */
    char *name;

    /* Pointer to zero-terminated list of pa_types which this module can
     * provide services for. */
    krb5_preauthtype *pa_type_list;

    /* Pointer to zero-terminated list of enc_types which this module claims
     * to add support for. */
    krb5_enctype *enctype_list;

    /* Per-module initialization/cleanup.  The init function is called
     * by libkrb5 when the module is loaded, and the fini function is
     * called before the module is unloaded.  Both are optional and
     * may be called multiple times in case the module is used in
     * multiple contexts.*/
    krb5_error_code (*init)(krb5_context, krb5_preauthtype, void **);
    void (*fini)(krb5_context, krb5_preauthtype, void *);
    /* A callback which returns flags indicating if the module is a "real" or
     * an "info" mechanism, and so on.  This function is called for each entry
     * in the client_pa_type_list. */
    int (*flags)(krb5_context, krb5_preauthtype);
    /* Clean up a client context.  Can be NULL. */
    void (*cleanup)(krb5_context context, void *module_context,
		    void **request_context);
    /* Client function which processes server-supplied data in pa_data,
     * returns created data in out_pa_data, storing any of its own state in
     * client_context if data for the associated preauthentication type is
     * needed.  It is also called after the AS-REP is received if the AS-REP
     * includes preauthentication data of the associated type.
     * NOTE! the encoded_previous_request will be NULL the first time this
     * function is called, because it is expected to only ever contain the data
     * obtained from a previous call to this function. */
    krb5_error_code (*process)(krb5_context context,
			       void *module_context,
			       void **request_context,
			       krb5_kdc_req *request,
			       krb5_data *encoded_request_body,
			       krb5_data *encoded_previous_request,
			       krb5_pa_data *pa_data,
			       krb5_prompter_fct prompter,
			       void *prompter_data,
			       preauth_get_as_key_proc gak_fct,
			       krb5_data *salt, krb5_data *s2kparams,
			       void *gak_data,
			       krb5_keyblock *as_key,
			       krb5_pa_data **out_pa_data);
    /* Client function which can attempt to use e-data in the error response to
     * try to recover from the given error.  If this function is not NULL, and
     * it stores data in out_pa_data which is different data from the contents
     * of in_pa_data, then the client library will retransmit the request. */
    krb5_error_code (*tryagain)(krb5_context context,
				void *module_context,
				void **request_context,
				krb5_kdc_req *request,
				krb5_data *encoded_request_body,
				krb5_error *error,
				krb5_pa_data *in_pa_data,
				krb5_pa_data **out_pa_data);
} krb5plugin_preauth_client_ftable_v0;

/*
 * The function table / structure which a preauth server module must export as
 * "preauthentication_server_0".  NOTE: replace "0" with "1" for the type and
 * variable names if this gets picked up by upstream.  If the interfaces work
 * correctly, future versions of the table will add either more callbacks or
 * more arguments to callbacks, and in both cases we'll be able to wrap the v0
 * functions.
 */
typedef struct krb5plugin_preauth_server_ftable_v0 {
    /* Not-usually-visible name. */
    char *name;

    /* Pointer to zero-terminated list of pa_types which this module can
     * provide services for. */
    krb5_preauthtype *pa_type_list;

    /* Per-module initialization/cleanup.  The init function is called by the
     * KDC when the module is loaded, and the fini function is called before
     * the module is unloaded.  Both are optional. */
    krb5_error_code (*init_proc)(krb5_context, krb5_preauthtype, void **);
    void (*fini_proc)(krb5_context, krb5_preauthtype, void *);
    /* Return the flags which the KDC should use for this module.  This is a
     * callback instead of a static value because the module may or may not
     * wish to count itself as a hardware preauthentication module (in other
     * words, the flags may be affected by the configuration, for example if a
     * site administrator can force a particular preauthentication type to be
     * supported using only hardware).  This function is called for each entry
     * entry in the server_pa_type_list. */
    int (*flags_proc)(krb5_context, krb5_preauthtype);
    /* Get preauthentication data to send to the client as part of the "you
     * need to use preauthentication" error.  The module doesn't need to
     * actually provide data if the protocol doesn't require it, but it should
     * return either zero or non-zero to control whether its padata type is
     * included in the list which is sent back to the client.  Is not allowed
     * to create a context because we have no guarantee that the client will
     * ever call again (or that it will hit this server if it does), in which
     * case a context might otherwise hang around forever. */
    krb5_error_code (*edata_proc)(krb5_context, krb5_kdc_req *request,
				  struct _krb5_db_entry_new *client,
				  struct _krb5_db_entry_new *server,
				  preauth_get_entry_data_proc,
				  void *pa_module_context,
				  krb5_pa_data *data);
    /* Verify preauthentication data sent by the client, setting the
     * TKT_FLG_PRE_AUTH or TKT_FLG_HW_AUTH flag in the enc_tkt_reply's "flags"
     * field as appropriate, and returning nonzero on failure.  Can create
     * context data for consumption by the return_proc or freepa_proc below. */
    krb5_error_code (*verify_proc)(krb5_context,
   				   struct _krb5_db_entry_new *client,
				   krb5_data *req_pkt,
				   krb5_kdc_req *request,
				   krb5_enc_tkt_part *enc_tkt_reply,
				   krb5_pa_data *data,
				   preauth_get_entry_data_proc,
				   void *pa_module_context,
				   void **pa_request_context);
    /* Generate preauthentication response data to send to the client as part
     * of the AS-REP.  If it needs to override the key which is used to encrypt
     * the response, it can do so.  The module is expected (but not required,
     * if a freepa_proc is also provided) to free any context data it saved in
     * "request_pa_context". */
    krb5_error_code (*return_proc)(krb5_context, krb5_pa_data * padata,
				   struct _krb5_db_entry_new *client,
				   krb5_data *req_pkt,
				   krb5_kdc_req *request,
				   krb5_kdc_rep *reply,
				   struct _krb5_key_data *client_keys,
				   krb5_keyblock *encrypting_key,
				   krb5_pa_data **send_pa,
				   preauth_get_entry_data_proc,
				   void *pa_module_context,
				   void **pa_request_context);
    /* Free up the server-side per-request context, in cases where
     * server_return_proc() didn't or for whatever reason was not called.  Can
     * be NULL. */
    krb5_error_code (*freepa_reqcontext_proc)(krb5_context,
   					      void *pa_module_context,
					      void **request_pa_context);
} krb5plugin_preauth_server_ftable_v0;
#endif /* KRB5_PREAUTH_PLUGIN_H_INCLUDED */
