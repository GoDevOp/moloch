/******************************************************************************/
/* writer-inplace.c  -- Writer that doesn't actually write pcap instead using
 *                      location of reading
 *
 * Copyright 2012-2016 AOL Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this Software except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define _FILE_OFFSET_BITS 64
#include "moloch.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>

extern MolochConfig_t        config;


LOCAL GHashTable           *filePtr2Id;
MOLOCH_LOCK_DEFINE(filePtr2Id);

/******************************************************************************/
uint32_t writer_inplace_queue_length()
{
    return 0;
}
/******************************************************************************/
void writer_inplace_flush(gboolean UNUSED(all))
{
}
/******************************************************************************/
void writer_inplace_exit()
{
}
/******************************************************************************/
long writer_inplace_create(MolochPacket_t * const packet)
{
    struct stat st;

    stat(packet->readerName, &st);

    uint32_t outputId;
    moloch_db_create_file(packet->ts.tv_sec, packet->readerName, st.st_size, 1, &outputId);
    g_hash_table_insert(filePtr2Id, packet->readerName, (gpointer)(long)outputId);
    return outputId;
}

/******************************************************************************/
void
writer_inplace_write(MolochPacket_t * const packet)
{
    MOLOCH_LOCK(filePtr2Id);
    long outputId = (long)g_hash_table_lookup(filePtr2Id, packet->readerName);
    if (!outputId)
        outputId = writer_inplace_create(packet);
    MOLOCH_UNLOCK(filePtr2Id);

    packet->writerFileNum = outputId;
    packet->writerFilePos = packet->readerFilePos;
}
/******************************************************************************/
void
writer_inplace_write_dryrun(MolochPacket_t * const packet)
{
    packet->writerFilePos = packet->readerFilePos;
}
/******************************************************************************/
char *
writer_inplace_name() {
    return "hmm";
}
/******************************************************************************/
void writer_inplace_init(char *UNUSED(name))
{
    moloch_writer_queue_length = writer_inplace_queue_length;
    moloch_writer_flush        = writer_inplace_flush;
    moloch_writer_exit         = writer_inplace_exit;
    if (config.dryRun)
        moloch_writer_write    = writer_inplace_write_dryrun;
    else
        moloch_writer_write    = writer_inplace_write;
    moloch_writer_name         = writer_inplace_name;

    filePtr2Id = g_hash_table_new(g_direct_hash, g_direct_equal);
}
