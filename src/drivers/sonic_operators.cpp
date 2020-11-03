#include "sonic_operators.h"

#include <swss/schema.h>
#include <swss/table.h>
#include <swss/producerstatetable.h>
#include <swss/dbconnector.h>
#include <swss/select.h>
#include <swss/subscriberstatetable.h>

#include <vector>
#include <map>
#include <algorithm>
#include <iterator>
#include <deque>
#include <memory>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

// select() function timeout retry time, in millisecond
constexpr int SELECT_TIMEOUT = 2000;

// Retry times to counter db
constexpr unsigned int RETRY_TIMES = 20;

// Retry interval to counter db, in millisecond
constexpr unsigned int RETRY_INTERVAL = 100;

class select_guard
{
private:
    swss::Selectable * m_selectable;
    swss::Select * m_selector;
public:
    select_guard(swss::Selectable * selectable, swss::Select * selector) :
        m_selectable(selectable),
        m_selector(selector)
    {
        if (m_selector != nullptr && m_selectable != nullptr)
        {
            m_selector->addSelectable(m_selectable);
        }
    }

    ~select_guard()
    {
        if (m_selector != nullptr && m_selectable != nullptr)
        {
            m_selector->removeSelectable(m_selectable);
        }
    }
};

class sonic_db_manager{
private:
    swss::DBConnector m_app_db;
    swss::DBConnector m_state_db;
    swss::DBConnector m_counters_db;

    std::map<std::string, swss::Table> m_tables_in_counter_db;
    std::map<std::string, swss::ProducerStateTable> m_producer_state_tables_in_app_db;
    std::map<std::string, swss::SubscriberStateTable> m_subscriber_state_tables_in_state_db;
    std::map<std::string, swss::Table> m_tables_in_state_db;

    swss::Select m_selector;

    template<typename TableMap>
    auto & get_table(TableMap & tables, swss::DBConnector & db, const std::string & table_name)
    {
        return tables.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(table_name),
                std::forward_as_tuple(&db, table_name)).first->second;
    }

    std::string get_counter_id(const std::string & obj_key)
    {
        std::vector<swss::FieldValueTuple> temp;
        auto & map_table = get_table(m_tables_in_counter_db, m_counters_db, COUNTERS_MACSEC_NAME_MAP);
        if (!map_table.get("", temp))
        {
            return "";
        }
        auto id = std::find_if(
            temp.begin(),
            temp.end(),
            [&](const swss::FieldValueTuple & fvt){
                return fvField(fvt) == obj_key;
            });
        if (id == temp.end())
        {
            return "";
        }
        return id->second;
    }

    bool meet_expectation(
        const std::string & op,
        const sonic_db_name_value_pair * pairs,
        unsigned int pair_count,
        const swss::KeyOpFieldsValuesTuple & entry) const
    {
        if (op.empty() || op != kfvOp(entry))
        {
            return false;
        }
        if (pairs == nullptr || pair_count == 0)
        {
            if (op == DEL_COMMAND)
            {
                return true;
            }
            else
            {
                return !kfvFieldsValues(entry).empty();
            }
            
        }
        auto values = kfvFieldsValues(entry);
        for (unsigned int i = 0; i < pair_count; i++)
        {
            if (pairs[i].name == nullptr)
            {
                continue;
            }
            auto value = std::find_if(
                values.begin(),
                values.end(),
                [&](const swss::FieldValueTuple & fvt)
                {
                    return pairs[i].name == fvField(fvt);
                });
            if (
                (value == values.end())
                || (
                    (pairs[i].value != nullptr)
                    && (value->second != pairs[i].value)
                    )
                )
            {
                return false;
            }
        }
        return true;
    }

public:
    sonic_db_manager():
        m_app_db("APPL_DB", 0),
        m_state_db("STATE_DB", 0),
        m_counters_db("COUNTERS_DB", 0)
        {
        }

    int set(
        int db_id,
        const std::string & table_name,
        const std::string & key,
        const struct sonic_db_name_value_pair * pairs,
        unsigned int pair_count)
    {
        if (db_id == APPL_DB)
        {
            auto & table = get_table(m_producer_state_tables_in_app_db, m_app_db, table_name);
            std::vector<swss::FieldValueTuple> values;
            if (pairs)
            {
                std::transform(
                    pairs,
                    pairs + pair_count,
                    std::back_inserter(values),
                    [](const sonic_db_name_value_pair & pair)
                    {
                        return std::make_pair(pair.name, pair.value ? pair.value : "");
                    });
            }
            table.set(key, values);
            return SONIC_DB_SUCCESS;
        }
        else
        {
            return SONIC_DB_FAIL;
        }
    }

    int get(
        int db_id,
        const std::string & table_name,
        const std::string & key,
        std::vector<swss::FieldValueTuple> & pairs)
    {
        pairs.clear();
        if (db_id == STATE_DB)
        {
            auto & table = get_table(m_tables_in_state_db, m_state_db, table_name);
            if(!table.get(key, pairs))
            {
                return SONIC_DB_FAIL;
            }
            return SONIC_DB_SUCCESS;
        }
        else
        {
            return SONIC_DB_FAIL;
        }
    }

    int get(
        int db_id,
        const std::string & table_name,
        const std::string & key,
        struct sonic_db_name_value_pairs * pairs)
    {
        std::vector<swss::FieldValueTuple> result;
        if (get(db_id, table_name, key, result) != SONIC_DB_SUCCESS)
        {
            return SONIC_DB_FAIL;
        }
        // Copy the query result to the output
        pairs->pairs = 
            reinterpret_cast<struct sonic_db_name_value_pair *>(
                realloc(pairs->pairs, sizeof(sonic_db_name_value_pair) * result.size())
            );
        if (pairs->pairs == nullptr)
        {
            return SONIC_DB_FAIL;
        }
        for (size_t i = 0; i < result.size(); i++)
        {
            char * name = reinterpret_cast<char *>(malloc(result[i].first.length() + 1));
            memcpy(name, result[i].first.data(), result[i].first.length() + 1);
            pairs->pairs[pairs->pair_count].name = name;
            char * value = reinterpret_cast<char *>(malloc(result[i].second.length() + 1));
            memcpy(value, result[i].first.data(), result[i].second.length() + 1);
            pairs->pairs[pairs->pair_count].value = value;
        }
        return SONIC_DB_SUCCESS;
    }

    int del(
        int db_id,
        const std::string & table_name,
        const std::string & key)
    {
        if (db_id == APPL_DB)
        {
            auto & table = get_table(m_producer_state_tables_in_app_db, m_app_db, table_name);
            table.del(key);
            return SONIC_DB_SUCCESS;
        }
        else
        {
            return SONIC_DB_FAIL;
        }
    }

    int wait(
    int db_id,
    const std::string & table_name,
    const std::string & op,
    const std::string & key,
    const struct sonic_db_name_value_pair * pairs,
    unsigned int pair_count)
    {
        // Subscribe the target table
        swss::ConsumerTableBase * consumer = nullptr;
        std::unique_ptr<select_guard> guarder;
        if (db_id == STATE_DB)
        {
            consumer = &get_table(m_subscriber_state_tables_in_state_db, m_state_db, table_name);
            guarder.reset(new select_guard(consumer, &m_selector));
        }
        else
        {
            return SONIC_DB_FAIL;
        }
        if (consumer == nullptr)
        {
            return SONIC_DB_FAIL;
        }

        // Proactively query the target table to avoid that 
        // the target table was updated before the subscription
        // which causes that the update cannot be fetched
        swss::KeyOpFieldsValuesTuple result;
        get(db_id, table_name, key, kfvFieldsValues(result));
        kfvOp(result) = kfvFieldsValues(result).empty() ? DEL_COMMAND : SET_COMMAND;
        if (meet_expectation(op, pairs, pair_count, result))
        {
            return SONIC_DB_SUCCESS;
        }

        // Fetch the update
        int ret = 0;
        while(true)
        {
            swss::Selectable *sel = nullptr;
            ret = m_selector.select(&sel, SELECT_TIMEOUT);
            if (ret == swss::Select::ERROR)
            {
                return SONIC_DB_FAIL;
            }
            if (ret == swss::Select::TIMEOUT)
            {
                return SONIC_DB_FAIL;
            }
            std::deque<swss::KeyOpFieldsValuesTuple> entries;
            consumer->pops(entries);
            for (auto & entry : entries)
            {
                if (meet_expectation(op, pairs, pair_count, entry))
                {
                    return SONIC_DB_SUCCESS;
                }
            }
        };
        return SONIC_DB_SUCCESS;
    }

    int get_counter(
        const std::string & table_name,
        const std::string & key,
        const std::string & field,
        unsigned long long * counter)
    {
        std::vector<swss::FieldValueTuple> result;
        const std::string id = get_counter_id(key);
        if (id.empty())
        {
            return SONIC_DB_FAIL;
        }
        // Find counter from counter db
        auto & counter_table = get_table(m_tables_in_counter_db, m_counters_db, table_name);
        auto retry_time = RETRY_TIMES;
        while (retry_time -- > 0)
        {
            if (!counter_table.get(id, result))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_INTERVAL));
                continue;
            }
            auto value = std::find_if(
                result.begin(),
                result.end(),
                [&](const swss::FieldValueTuple & fvt)
                {
                    return field == fvField(fvt);
                });
            if (value == result.end())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_INTERVAL));
                continue;
            }
            std::stringstream(fvValue(*value)) >> *counter;
            return SONIC_DB_SUCCESS;
        }
        return SONIC_DB_FAIL;
    }

    int del_counter(
        const std::string & table_name,
        const std::string & key)
    {
        const std::string id = get_counter_id(key);
        if (id.empty())
        {
            return SONIC_DB_FAIL;
        }
        if (m_tables_in_counter_db.erase(id) == 0)
        {
            return SONIC_DB_FAIL;
        }
        return SONIC_DB_SUCCESS;
    }
};

sonic_db_handle sonic_db_get_manager()
{
    thread_local sonic_db_manager manager;
    return &manager;
}

int sonic_db_set(
    sonic_db_handle sonic_manager,
    int db_id,
    const char * table_name,
    const char * key,
    const struct sonic_db_name_value_pair * pairs,
    unsigned int pair_count)
{
    sonic_db_manager * manager = reinterpret_cast<sonic_db_manager *>(sonic_manager);
    if (manager == nullptr)
    {
        return SONIC_DB_FAIL;
    }
    return manager->set(db_id, table_name, key, pairs, pair_count);
}

int sonic_db_get(
    sonic_db_handle sonic_manager,
    int db_id,
    const char * table_name,
    const char * key,
    struct sonic_db_name_value_pairs * pairs)
{
    sonic_db_manager * manager = reinterpret_cast<sonic_db_manager *>(sonic_manager);
    if (manager == nullptr)
    {
        return SONIC_DB_FAIL;
    }
    return manager->get(db_id, table_name, key, pairs);
}

int sonic_db_del(
    sonic_db_handle sonic_manager,
    int db_id,
    const char * table_name,
    const char * key)
{
    sonic_db_manager * manager = reinterpret_cast<sonic_db_manager *>(sonic_manager);
    if (manager == nullptr)
    {
        return SONIC_DB_FAIL;
    }
    return manager->del(db_id, table_name, key);
}

int sonic_db_wait(
    sonic_db_handle sonic_manager,
    int db_id,
    const char * table,
    const char * op,
    const char * key,
    const struct sonic_db_name_value_pair * pairs,
    unsigned int pair_count)
{
    sonic_db_manager * manager = reinterpret_cast<sonic_db_manager *>(sonic_manager);
    if (manager == nullptr)
    {
        return SONIC_DB_FAIL;
    }
    return manager->wait(db_id, table, op, key, pairs, pair_count);
}

int sonic_db_get_counter(
    sonic_db_handle sonic_manager,
    const char * table_name,
    const char * key,
    const char * field,
    unsigned long long * counter)
{
    sonic_db_manager * manager = reinterpret_cast<sonic_db_manager *>(sonic_manager);
    if (manager == nullptr)
    {
        return SONIC_DB_FAIL;
    }
    return manager->get_counter(table_name, key, field, counter);
}

int sonic_db_del_counter(
    sonic_db_handle sonic_manager,
    const char * table_name,
    const char * key)
{
    sonic_db_manager * manager = reinterpret_cast<sonic_db_manager *>(sonic_manager);
    if (manager == nullptr)
    {
        return SONIC_DB_FAIL;
    }
    return manager->del_counter(table_name, key);
}


struct sonic_db_name_value_pairs * sonic_db_malloc_name_value_pairs()
{
    struct sonic_db_name_value_pairs * pairs = reinterpret_cast<struct sonic_db_name_value_pairs *>(
        malloc(sizeof(struct sonic_db_name_value_pairs))
    );
    if (pairs == nullptr)
    {
        return nullptr;
    }
    pairs->pair_count = 0;
    pairs->pairs = UNSET_POINTER;
    return reinterpret_cast<struct sonic_db_name_value_pairs *>(pairs);
}

void sonic_db_free_name_value_pairs(struct sonic_db_name_value_pairs * pairs)
{
    if (pairs == nullptr)
    {
        return;
    }
    for (unsigned int i = 0; i < pairs->pair_count; i++)
    {
        if (pairs->pairs[i].name != UNSET_POINTER)
        {
            free((char *)pairs->pairs[i].name);
        }
        if (pairs->pairs[i].value != UNSET_POINTER)
        {
            free((char *)pairs->pairs[i].value);
        }
    }
    free(pairs);
}
