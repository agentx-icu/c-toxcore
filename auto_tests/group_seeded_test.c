/* Seeded group creation: the same seed yields the same Chat ID on
 * independent Tox instances, different seeds differ, and tox_group_new
 * stays random. No network is needed.
 */

#include <stdio.h>
#include <string.h>

#include "../testing/misc_tools.h"
#include "../toxcore/ccompat.h"
#include "../toxcore/tox.h"
#include "../toxcore/tox_private.h"
#include "auto_test_support.h"
#include "check_compat.h"

static uint32_t new_seeded(Tox *tox, const uint8_t *seed, Tox_Err_Group_New *err)
{
    const uint8_t name[] = "seeded";
    const uint8_t nick[] = "peer";
    return tox_group_new_with_seed(tox, TOX_GROUP_PRIVACY_STATE_PUBLIC, name, sizeof(name) - 1,
                                   nick, sizeof(nick) - 1, seed, err);
}

static void chat_id_of(const Tox *tox, uint32_t group_number, uint8_t *chat_id)
{
    Tox_Err_Group_State_Query err;
    ck_assert(tox_group_get_chat_id(tox, group_number, chat_id, &err));
    ck_assert(err == TOX_ERR_GROUP_STATE_QUERY_OK);
}

int main(void)
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    ck_assert(tox_group_seed_size() == TOX_GROUP_SEED_SIZE);

    Tox *tox1 = tox_new_log(nullptr, nullptr, nullptr);
    Tox *tox2 = tox_new_log(nullptr, nullptr, nullptr);
    ck_assert(tox1 != nullptr && tox2 != nullptr);

    uint8_t seed_a[TOX_GROUP_SEED_SIZE];
    uint8_t seed_b[TOX_GROUP_SEED_SIZE];
    memset(seed_a, 0xA5, sizeof(seed_a));
    memset(seed_b, 0x5A, sizeof(seed_b));

    Tox_Err_Group_New err;

    /* Same seed on two independent instances -> same Chat ID. */
    const uint32_t g1 = new_seeded(tox1, seed_a, &err);
    ck_assert_msg(err == TOX_ERR_GROUP_NEW_OK, "tox1 seeded new failed: %d", err);
    const uint32_t g2 = new_seeded(tox2, seed_a, &err);
    ck_assert_msg(err == TOX_ERR_GROUP_NEW_OK, "tox2 seeded new failed: %d", err);

    uint8_t id1[TOX_GROUP_CHAT_ID_SIZE];
    uint8_t id2[TOX_GROUP_CHAT_ID_SIZE];
    chat_id_of(tox1, g1, id1);
    chat_id_of(tox2, g2, id2);
    ck_assert_msg(memcmp(id1, id2, TOX_GROUP_CHAT_ID_SIZE) == 0, "same seed must give the same chat id");

    /* A different seed -> a different Chat ID. */
    const uint32_t g3 = new_seeded(tox1, seed_b, &err);
    ck_assert(err == TOX_ERR_GROUP_NEW_OK);
    uint8_t id3[TOX_GROUP_CHAT_ID_SIZE];
    chat_id_of(tox1, g3, id3);
    ck_assert_msg(memcmp(id1, id3, TOX_GROUP_CHAT_ID_SIZE) != 0, "different seeds must give different chat ids");

    /* tox_group_new stays random: two plain groups differ from each other and from the seeded ones. */
    const uint8_t name[] = "plain";
    const uint8_t nick[] = "peer";
    const uint32_t g4 = tox_group_new(tox2, TOX_GROUP_PRIVACY_STATE_PUBLIC, name, sizeof(name) - 1, nick, sizeof(nick) - 1, &err);
    ck_assert(err == TOX_ERR_GROUP_NEW_OK);
    const uint32_t g5 = tox_group_new(tox2, TOX_GROUP_PRIVACY_STATE_PUBLIC, name, sizeof(name) - 1, nick, sizeof(nick) - 1, &err);
    ck_assert(err == TOX_ERR_GROUP_NEW_OK);
    uint8_t id4[TOX_GROUP_CHAT_ID_SIZE];
    uint8_t id5[TOX_GROUP_CHAT_ID_SIZE];
    chat_id_of(tox2, g4, id4);
    chat_id_of(tox2, g5, id5);
    ck_assert(memcmp(id4, id5, TOX_GROUP_CHAT_ID_SIZE) != 0);
    ck_assert(memcmp(id4, id1, TOX_GROUP_CHAT_ID_SIZE) != 0);

    /* A null seed is rejected without creating a group. */
    const uint32_t before = tox_group_get_number_groups(tox1);
    const uint32_t g6 = new_seeded(tox1, nullptr, &err);
    ck_assert(g6 == UINT32_MAX);
    ck_assert(err == TOX_ERR_GROUP_NEW_INIT);
    ck_assert(tox_group_get_number_groups(tox1) == before);

    /* The usual argument checks still apply. */
    const uint32_t g7 = tox_group_new_with_seed(tox1, TOX_GROUP_PRIVACY_STATE_PUBLIC, name, 0, nick, sizeof(nick) - 1, seed_a, &err);
    ck_assert(g7 == UINT32_MAX);
    ck_assert(err == TOX_ERR_GROUP_NEW_EMPTY);

    tox_kill(tox2);
    tox_kill(tox1);
    return 0;
}
