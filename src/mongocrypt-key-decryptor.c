/*
 * Copyright 2019-present MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "kms_message/kms_request_opt.h"

#include "mongocrypt-buffer-private.h"
#include "mongocrypt-key-decryptor.h"
#include "mongocrypt-key-decryptor-private.h"

void
_mongocrypt_key_decryptor_init (mongocrypt_key_decryptor_t *kd,
                                _mongocrypt_buffer_t *key_material,
                                void *ctx)
{
   kms_request_opt_t *opt;
   /* create the KMS request. */
   opt = kms_request_opt_new ();
   /* TODO: we might want to let drivers control whether or not we send
      * Connection: close header. Unsure right now. */
   kms_request_opt_set_connection_close (opt, true);
   kd->req =
      kms_decrypt_request_new (key_material->data, key_material->len, opt);
   kd->parser = kms_response_parser_new ();
   kd->ctx = ctx;
   kd->msg = mongocrypt_binary_new ();
   kms_request_opt_destroy (opt);
}

const mongocrypt_binary_t *
mongocrypt_key_decryptor_msg (mongocrypt_key_decryptor_t *kd,
                              const mongocrypt_opts_t *opts,
                              mongocrypt_status_t *status)
{
   if (kd->msg->data) {
      return kd->msg;
   }
   kd->msg->data = (uint8_t *) kms_request_get_signed (kd->req);
   kd->msg->len = strlen ((char *) kd->msg->data);
   return kd->msg;
}


int
mongocrypt_key_decryptor_bytes_needed (mongocrypt_key_decryptor_t *kd,
                                       uint32_t max_bytes)
{
   return kms_response_parser_wants_bytes (kd->parser, (int32_t) max_bytes);
}


bool
mongocrypt_key_decryptor_feed (mongocrypt_key_decryptor_t *kd,
                               mongocrypt_binary_t *bytes,
                               mongocrypt_status_t *status)
{
   /* TODO: KMS error handling in CDRIVER-3000? */
   kms_response_parser_feed (kd->parser, bytes->data, bytes->len);
   return true;
}


void
_mongocrypt_key_decryptor_cleanup (mongocrypt_key_decryptor_t *kd)
{
   if (!kd) {
      return;
   }
   kms_request_destroy (kd->req);
   kms_response_parser_destroy (kd->parser);
   mongocrypt_binary_destroy (kd->msg);
}