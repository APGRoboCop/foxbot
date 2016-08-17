/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "util.h"
#include <cbase.h>
#include <saverestore.h>
#include <client.h>
#include <decals.h>
#include <gamerules.h>
#include <game.h>
#include "bot.h"

void EntvarsKeyvalue(entvars_t* pev, KeyValueData* pkvd);

extern TYPEDESCRIPTION gEntvarsDescription[] = {
    DEFINE_ENTITY_FIELD(classname, FIELD_STRING), DEFINE_ENTITY_GLOBAL_FIELD(globalname, FIELD_STRING),

    DEFINE_ENTITY_FIELD(origin, FIELD_POSITION_VECTOR), DEFINE_ENTITY_FIELD(oldorigin, FIELD_POSITION_VECTOR),
    DEFINE_ENTITY_FIELD(velocity, FIELD_VECTOR), DEFINE_ENTITY_FIELD(basevelocity, FIELD_VECTOR),
    DEFINE_ENTITY_FIELD(movedir, FIELD_VECTOR),

    DEFINE_ENTITY_FIELD(angles, FIELD_VECTOR), DEFINE_ENTITY_FIELD(avelocity, FIELD_VECTOR),
    DEFINE_ENTITY_FIELD(punchangle, FIELD_VECTOR), DEFINE_ENTITY_FIELD(v_angle, FIELD_VECTOR),
    DEFINE_ENTITY_FIELD(fixangle, FIELD_FLOAT), DEFINE_ENTITY_FIELD(idealpitch, FIELD_FLOAT),
    DEFINE_ENTITY_FIELD(pitch_speed, FIELD_FLOAT), DEFINE_ENTITY_FIELD(ideal_yaw, FIELD_FLOAT),
    DEFINE_ENTITY_FIELD(yaw_speed, FIELD_FLOAT),

    DEFINE_ENTITY_FIELD(modelindex, FIELD_INTEGER), DEFINE_ENTITY_GLOBAL_FIELD(model, FIELD_MODELNAME),

    DEFINE_ENTITY_FIELD(viewmodel, FIELD_MODELNAME), DEFINE_ENTITY_FIELD(weaponmodel, FIELD_MODELNAME),

    DEFINE_ENTITY_FIELD(absmin, FIELD_POSITION_VECTOR), DEFINE_ENTITY_FIELD(absmax, FIELD_POSITION_VECTOR),
    DEFINE_ENTITY_GLOBAL_FIELD(mins, FIELD_VECTOR), DEFINE_ENTITY_GLOBAL_FIELD(maxs, FIELD_VECTOR),
    DEFINE_ENTITY_GLOBAL_FIELD(size, FIELD_VECTOR),

    DEFINE_ENTITY_FIELD(ltime, FIELD_TIME), DEFINE_ENTITY_FIELD(nextthink, FIELD_TIME),

    DEFINE_ENTITY_FIELD(solid, FIELD_INTEGER), DEFINE_ENTITY_FIELD(movetype, FIELD_INTEGER),

    DEFINE_ENTITY_FIELD(skin, FIELD_INTEGER), DEFINE_ENTITY_FIELD(body, FIELD_INTEGER),
    DEFINE_ENTITY_FIELD(effects, FIELD_INTEGER),

    DEFINE_ENTITY_FIELD(gravity, FIELD_FLOAT), DEFINE_ENTITY_FIELD(friction, FIELD_FLOAT),
    DEFINE_ENTITY_FIELD(light_level, FIELD_FLOAT),

    DEFINE_ENTITY_FIELD(frame, FIELD_FLOAT), DEFINE_ENTITY_FIELD(scale, FIELD_FLOAT),
    DEFINE_ENTITY_FIELD(sequence, FIELD_INTEGER), DEFINE_ENTITY_FIELD(animtime, FIELD_TIME),
    DEFINE_ENTITY_FIELD(framerate, FIELD_FLOAT), DEFINE_ENTITY_FIELD(controller, FIELD_INTEGER),
    DEFINE_ENTITY_FIELD(blending, FIELD_INTEGER),

    DEFINE_ENTITY_FIELD(rendermode, FIELD_INTEGER), DEFINE_ENTITY_FIELD(renderamt, FIELD_FLOAT),
    DEFINE_ENTITY_FIELD(rendercolor, FIELD_VECTOR), DEFINE_ENTITY_FIELD(renderfx, FIELD_INTEGER),

    DEFINE_ENTITY_FIELD(health, FIELD_FLOAT), DEFINE_ENTITY_FIELD(frags, FIELD_FLOAT),
    DEFINE_ENTITY_FIELD(weapons, FIELD_INTEGER), DEFINE_ENTITY_FIELD(takedamage, FIELD_FLOAT),

    DEFINE_ENTITY_FIELD(deadflag, FIELD_FLOAT), DEFINE_ENTITY_FIELD(view_ofs, FIELD_VECTOR),
    DEFINE_ENTITY_FIELD(button, FIELD_INTEGER), DEFINE_ENTITY_FIELD(impulse, FIELD_INTEGER),

    DEFINE_ENTITY_FIELD(chain, FIELD_EDICT), DEFINE_ENTITY_FIELD(dmg_inflictor, FIELD_EDICT),
    DEFINE_ENTITY_FIELD(enemy, FIELD_EDICT), DEFINE_ENTITY_FIELD(aiment, FIELD_EDICT),
    DEFINE_ENTITY_FIELD(owner, FIELD_EDICT), DEFINE_ENTITY_FIELD(groundentity, FIELD_EDICT),

    DEFINE_ENTITY_FIELD(spawnflags, FIELD_INTEGER), DEFINE_ENTITY_FIELD(flags, FIELD_FLOAT),

    DEFINE_ENTITY_FIELD(colormap, FIELD_INTEGER), DEFINE_ENTITY_FIELD(team, FIELD_INTEGER),

    DEFINE_ENTITY_FIELD(max_health, FIELD_FLOAT), DEFINE_ENTITY_FIELD(teleport_time, FIELD_TIME),
    DEFINE_ENTITY_FIELD(armortype, FIELD_FLOAT), DEFINE_ENTITY_FIELD(armorvalue, FIELD_FLOAT),
    DEFINE_ENTITY_FIELD(waterlevel, FIELD_INTEGER), DEFINE_ENTITY_FIELD(watertype, FIELD_INTEGER),

    // Having these fields be local to the individual levels makes it easier to test those levels individually.
    DEFINE_ENTITY_GLOBAL_FIELD(target, FIELD_STRING), DEFINE_ENTITY_GLOBAL_FIELD(targetname, FIELD_STRING),
    DEFINE_ENTITY_FIELD(netname, FIELD_STRING), DEFINE_ENTITY_FIELD(message, FIELD_STRING),

    DEFINE_ENTITY_FIELD(dmg_take, FIELD_FLOAT), DEFINE_ENTITY_FIELD(dmg_save, FIELD_FLOAT),
    DEFINE_ENTITY_FIELD(dmg, FIELD_FLOAT), DEFINE_ENTITY_FIELD(dmgtime, FIELD_TIME),

    DEFINE_ENTITY_FIELD(noise, FIELD_SOUNDNAME), DEFINE_ENTITY_FIELD(noise1, FIELD_SOUNDNAME),
    DEFINE_ENTITY_FIELD(noise2, FIELD_SOUNDNAME), DEFINE_ENTITY_FIELD(noise3, FIELD_SOUNDNAME),
    DEFINE_ENTITY_FIELD(speed, FIELD_FLOAT), DEFINE_ENTITY_FIELD(air_finished, FIELD_TIME),
    DEFINE_ENTITY_FIELD(pain_finished, FIELD_TIME), DEFINE_ENTITY_FIELD(radsuit_finished, FIELD_TIME),
};

#define ENTVARS_COUNT (sizeof(gEntvarsDescription) / sizeof(gEntvarsDescription[0]))

static int gSizes[FIELD_TYPECOUNT] = {
    sizeof(float),     // FIELD_FLOAT
    sizeof(int),       // FIELD_STRING
    sizeof(int),       // FIELD_ENTITY
    sizeof(int),       // FIELD_CLASSPTR
    sizeof(int),       // FIELD_EHANDLE
    sizeof(int),       // FIELD_entvars_t
    sizeof(int),       // FIELD_EDICT
    sizeof(float) * 3, // FIELD_VECTOR
    sizeof(float) * 3, // FIELD_POSITION_VECTOR
    sizeof(int*),      // FIELD_POINTER
    sizeof(int),       // FIELD_INTEGER
    sizeof(int*),      // FIELD_FUNCTION
    sizeof(int),       // FIELD_BOOLEAN
    sizeof(short),     // FIELD_SHORT
    sizeof(char),      // FIELD_CHARACTER
    sizeof(float),     // FIELD_TIME
    sizeof(int),       // FIELD_MODELNAME
    sizeof(int),       // FIELD_SOUNDNAME
};

void UTIL_Remove(CBaseEntity* pEntity)
{
    if(!pEntity)
        return;

    pEntity->UpdateOnRemove();
    pEntity->pev->flags |= FL_KILLME;
    pEntity->pev->targetname = 0;
}

void CSave::WriteVector(const char* pname, const Vector& value)
{
    WriteVector(pname, &value.x, 1);
}

void CSave::WriteVector(const char* pname, const float* value, int count)
{
    BufferHeader(pname, sizeof(float) * 3 * count);
    BufferData((const char*)value, sizeof(float) * 3 * count);
}

void CSave::WritePositionVector(const char* pname, const Vector& value)
{

    if(m_pdata && m_pdata->fUseLandmark) {
        Vector tmp = value - m_pdata->vecLandmarkOffset;
        WriteVector(pname, tmp);
    }

    WriteVector(pname, value);
}

void CSave::WritePositionVector(const char* pname, const float* value, int count)
{
    int i;
    Vector tmp, input;

    BufferHeader(pname, sizeof(float) * 3 * count);
    for(i = 0; i < count; i++) {
        Vector tmp(value[0], value[1], value[2]);

        if(m_pdata && m_pdata->fUseLandmark)
            tmp = tmp - m_pdata->vecLandmarkOffset;

        BufferData((const char*)&tmp.x, sizeof(float) * 3);
        value += 3;
    }
}

void CSave::WriteFunction(const char* pname, const int* data, int count)
{
    const char* functionName;

    functionName = NAME_FOR_FUNCTION(*data);
    if(functionName)
        BufferField(pname, strlen(functionName) + 1, functionName);
    else
        ALERT(at_error, "Invalid function pointer in entity!");
}

int CSave::WriteEntVars(const char* pname, entvars_t* pev)
{
    return WriteFields(pname, pev, gEntvarsDescription, ENTVARS_COUNT);
}

int CSave::WriteFields(const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount)
{
    int i, j, actualCount, emptyCount;
    TYPEDESCRIPTION* pTest;
    int entityArray[MAX_ENTITYARRAY];

    // Precalculate the number of empty fields
    emptyCount = 0;
    for(i = 0; i < fieldCount; i++) {
        void* pOutputData;
        pOutputData = ((char*)pBaseData + pFields[i].fieldOffset);
        if(DataEmpty((const char*)pOutputData, pFields[i].fieldSize * gSizes[pFields[i].fieldType]))
            emptyCount++;
    }

    // Empty fields will not be written, write out the actual number of fields to be written
    actualCount = fieldCount - emptyCount;
    WriteInt(pname, &actualCount, 1);

    for(i = 0; i < fieldCount; i++) {
        void* pOutputData;
        pTest = &pFields[i];
        pOutputData = ((char*)pBaseData + pTest->fieldOffset);

        // UNDONE: Must we do this twice?
        if(DataEmpty((const char*)pOutputData, pTest->fieldSize * gSizes[pTest->fieldType]))
            continue;

        switch(pTest->fieldType) {
        case FIELD_FLOAT:
            WriteFloat(pTest->fieldName, (float*)pOutputData, pTest->fieldSize);
            break;
        case FIELD_TIME:
            WriteTime(pTest->fieldName, (float*)pOutputData, pTest->fieldSize);
            break;
        case FIELD_MODELNAME:
        case FIELD_SOUNDNAME:
        case FIELD_STRING:
            WriteString(pTest->fieldName, (int*)pOutputData, pTest->fieldSize);
            break;
        case FIELD_CLASSPTR:
        case FIELD_EVARS:
        case FIELD_EDICT:
        case FIELD_ENTITY:
        case FIELD_EHANDLE:
            if(pTest->fieldSize > MAX_ENTITYARRAY)
                ALERT(at_error, "Can't save more than %d entities in an array!!!\n", MAX_ENTITYARRAY);
            for(j = 0; j < pTest->fieldSize; j++) {
                switch(pTest->fieldType) {
                case FIELD_EVARS:
                    entityArray[j] = EntityIndex(((entvars_t**)pOutputData)[j]);
                    break;
                case FIELD_CLASSPTR:
                    entityArray[j] = EntityIndex(((CBaseEntity**)pOutputData)[j]);
                    break;
                case FIELD_EDICT:
                    entityArray[j] = EntityIndex(((edict_t**)pOutputData)[j]);
                    break;
                case FIELD_ENTITY:
                    entityArray[j] = EntityIndex(((EOFFSET*)pOutputData)[j]);
                    break;
                case FIELD_EHANDLE:
                    entityArray[j] = EntityIndex((CBaseEntity*)(((EHANDLE*)pOutputData)[j]));
                    break;
                }
            }
            WriteInt(pTest->fieldName, entityArray, pTest->fieldSize);
            break;
        case FIELD_POSITION_VECTOR:
            WritePositionVector(pTest->fieldName, (float*)pOutputData, pTest->fieldSize);
            break;
        case FIELD_VECTOR:
            WriteVector(pTest->fieldName, (float*)pOutputData, pTest->fieldSize);
            break;

        case FIELD_BOOLEAN:
        case FIELD_INTEGER:
            WriteInt(pTest->fieldName, (int*)pOutputData, pTest->fieldSize);
            break;

        case FIELD_SHORT:
            WriteData(pTest->fieldName, 2 * pTest->fieldSize, ((char*)pOutputData));
            break;

        case FIELD_CHARACTER:
            WriteData(pTest->fieldName, pTest->fieldSize, ((char*)pOutputData));
            break;

        // For now, just write the address out, we're not going to change memory while doing this yet!
        case FIELD_POINTER:
            WriteInt(pTest->fieldName, (int*)(char*)pOutputData, pTest->fieldSize);
            break;

        case FIELD_FUNCTION:
            WriteFunction(pTest->fieldName, (int*)(char*)pOutputData, pTest->fieldSize);
            break;
        default:
            ALERT(at_error, "Bad field type\n");
        }
    }

    return 1;
}

void CSave::WriteData(const char* pname, int size, const char* pdata)
{
    BufferField(pname, size, pdata);
}

int CSave::DataEmpty(const char* pdata, int size)
{
    for(int i = 0; i < size; i++) {
        if(pdata[i])
            return 0;
    }
    return 1;
}

void CSave::BufferField(const char* pname, int size, const char* pdata)
{
    BufferHeader(pname, size);
    BufferData(pdata, size);
}

void CSave::BufferHeader(const char* pname, int size)
{
    short hashvalue = TokenHash(pname);
    if(size > 1 << (sizeof(short) * 8))
        ALERT(at_error, "CSave :: BufferHeader() size parameter exceeds 'short'!");
    BufferData((const char*)&size, sizeof(short));
    BufferData((const char*)&hashvalue, sizeof(short));
}

void CSave::BufferData(const char* pdata, int size)
{
    if(!m_pdata)
        return;

    if(m_pdata->size + size > m_pdata->bufferSize) {
        ALERT(at_error, "Save/Restore overflow!");
        m_pdata->size = m_pdata->bufferSize;
        return;
    }

    memcpy(m_pdata->pCurrentData, pdata, size);
    m_pdata->pCurrentData += size;
    m_pdata->size += size;
}

void CSave::WriteInt(const char* pname, const int* data, int count)
{
    BufferField(pname, sizeof(int) * count, (const char*)data);
}

void CSave::WriteFloat(const char* pname, const float* data, int count)
{
    BufferField(pname, sizeof(float) * count, (const char*)data);
}

void CSave::WriteTime(const char* pname, const float* data, int count)
{
    int i;
    Vector tmp, input;

    BufferHeader(pname, sizeof(float) * count);
    for(i = 0; i < count; i++) {
        float tmp = data[0];

        // Always encode time as a delta from the current time so it can be re-based if loaded in a new level
        // Times of 0 are never written to the file, so they will be restored as 0, not a relative time
        if(m_pdata)
            tmp -= m_pdata->time;

        BufferData((const char*)&tmp, sizeof(float));
        data++;
    }
}

void CSave::WriteString(const char* pname, const char* pdata)
{
#ifdef TOKENIZE
    short token = (short)TokenHash(pdata);
    WriteShort(pname, &token, 1);
#else
    BufferField(pname, strlen(pdata) + 1, pdata);
#endif
}

void CSave::WriteString(const char* pname, const int* stringId, int count)
{
    int i, size;

#ifdef TOKENIZE
    short token = (short)TokenHash(STRING(*stringId));
    WriteShort(pname, &token, 1);
#else
#if 0
	if ( count != 1 )
		ALERT( at_error, "No string arrays!\n" );
	WriteString( pname, (char *)STRING(*stringId) );
#endif

    size = 0;
    for(i = 0; i < count; i++)
        size += strlen(STRING(stringId[i])) + 1;

    BufferHeader(pname, size);
    for(i = 0; i < count; i++) {
        const char* pString = STRING(stringId[i]);
        BufferData(pString, strlen(pString) + 1);
    }
#endif
}

unsigned int CSaveRestoreBuffer::HashString(const char* pszToken)
{
    unsigned int hash = 0;

    while(*pszToken)
        hash = _rotr(hash, 4) ^ *pszToken++;

    return hash;
}

unsigned short CSaveRestoreBuffer::TokenHash(const char* pszToken)
{
    unsigned short hash = (unsigned short)(HashString(pszToken) % (unsigned)m_pdata->tokenCount);

#if _DEBUG
    static int tokensparsed = 0;
    tokensparsed++;
    if(!m_pdata->tokenCount || !m_pdata->pTokens)
        ALERT(at_error, "No token table array in TokenHash()!");
#endif

    for(int i = 0; i < m_pdata->tokenCount; i++) {
#if _DEBUG
        static qboolean beentheredonethat = FALSE;
        if(i > 50 && !beentheredonethat) {
            beentheredonethat = TRUE;
            ALERT(at_error, "CSaveRestoreBuffer :: TokenHash() is getting too full!");
        }
#endif

        int index = hash + i;
        if(index >= m_pdata->tokenCount)
            index -= m_pdata->tokenCount;

        if(!m_pdata->pTokens[index] || strcmp(pszToken, m_pdata->pTokens[index]) == 0) {
            m_pdata->pTokens[index] = (char*)pszToken;
            return index;
        }
    }

    // Token hash table full!!!
    // [Consider doing overflow table(s) after the main table & limiting linear hash table search]
    ALERT(at_error, "CSaveRestoreBuffer :: TokenHash() is COMPLETELY FULL!");
    return 0;
}

int CSaveRestoreBuffer::EntityIndex(CBaseEntity* pEntity)
{
    if(pEntity == NULL)
        return -1;
    return EntityIndex(pEntity->pev);
}

int CSaveRestoreBuffer::EntityIndex(entvars_t* pevLookup)
{
    if(pevLookup == NULL)
        return -1;
    return EntityIndex(ENT(pevLookup));
}

int CSaveRestoreBuffer::EntityIndex(EOFFSET eoLookup)
{
    return EntityIndex(ENT(eoLookup));
}

int CSaveRestoreBuffer::EntityIndex(edict_t* pentLookup)
{
    if(!m_pdata || pentLookup == NULL)
        return -1;

    int i;
    ENTITYTABLE* pTable;

    for(i = 0; i < m_pdata->tableCount; i++) {
        pTable = m_pdata->pTable + i;
        if(pTable->pent == pentLookup)
            return i;
    }
    return -1;
}

edict_t* CSaveRestoreBuffer::EntityFromIndex(int entityIndex)
{
    if(!m_pdata || entityIndex < 0)
        return NULL;

    int i;
    ENTITYTABLE* pTable;

    for(i = 0; i < m_pdata->tableCount; i++) {
        pTable = m_pdata->pTable + i;
        if(pTable->id == entityIndex)
            return pTable->pent;
    }
    return NULL;
}

void CSaveRestoreBuffer::BufferRewind(int size)
{
    if(!m_pdata)
        return;

    if(m_pdata->size < size)
        size = m_pdata->size;

    m_pdata->pCurrentData -= size;
    m_pdata->size -= size;
}

int CRestore::ReadField(void* pBaseData,
    TYPEDESCRIPTION* pFields,
    int fieldCount,
    int startField,
    int size,
    char* pName,
    void* pData)
{
    int i, j, stringCount, fieldNumber, entityIndex;
    TYPEDESCRIPTION* pTest;
    float time, timeData;
    Vector position;
    edict_t* pent;
    char* pString;

    time = 0;
    position = Vector(0, 0, 0);

    if(m_pdata) {
        time = m_pdata->time;
        if(m_pdata->fUseLandmark)
            position = m_pdata->vecLandmarkOffset;
    }

    for(i = 0; i < fieldCount; i++) {
        fieldNumber = (i + startField) % fieldCount;
        pTest = &pFields[fieldNumber];
        if(!stricmp(pTest->fieldName, pName)) {
            if(!m_global || !(pTest->flags & FTYPEDESC_GLOBAL)) {
                for(j = 0; j < pTest->fieldSize; j++) {
                    void* pOutputData = ((char*)pBaseData + pTest->fieldOffset + (j * gSizes[pTest->fieldType]));
                    void* pInputData = (char*)pData + j * gSizes[pTest->fieldType];

                    switch(pTest->fieldType) {
                    case FIELD_TIME:
                        timeData = *(float*)pInputData;
                        // Re-base time variables
                        timeData += time;
                        *((float*)pOutputData) = timeData;
                        break;
                    case FIELD_FLOAT:
                        *((float*)pOutputData) = *(float*)pInputData;
                        break;
                    case FIELD_MODELNAME:
                    case FIELD_SOUNDNAME:
                    case FIELD_STRING:
                        // Skip over j strings
                        pString = (char*)pData;
                        for(stringCount = 0; stringCount < j; stringCount++) {
                            while(*pString)
                                pString++;
                            pString++;
                        }
                        pInputData = pString;
                        if(strlen((char*)pInputData) == 0)
                            *((int*)pOutputData) = 0;
                        else {
                            int string;

                            string = ALLOC_STRING((char*)pInputData);

                            *((int*)pOutputData) = string;

                            if(!FStringNull(string) && m_precache) {
                                if(pTest->fieldType == FIELD_MODELNAME)
                                    PRECACHE_MODEL((char*)STRING(string));
                                else if(pTest->fieldType == FIELD_SOUNDNAME)
                                    PRECACHE_SOUND((char*)STRING(string));
                            }
                        }
                        break;
                    case FIELD_EVARS:
                        entityIndex = *(int*)pInputData;
                        pent = EntityFromIndex(entityIndex);
                        if(pent)
                            *((entvars_t**)pOutputData) = VARS(pent);
                        else
                            *((entvars_t**)pOutputData) = NULL;
                        break;
                    case FIELD_CLASSPTR:
                        entityIndex = *(int*)pInputData;
                        pent = EntityFromIndex(entityIndex);
                        if(pent)
                            *((CBaseEntity**)pOutputData) = CBaseEntity::Instance(pent);
                        else
                            *((CBaseEntity**)pOutputData) = NULL;
                        break;
                    case FIELD_EDICT:
                        entityIndex = *(int*)pInputData;
                        pent = EntityFromIndex(entityIndex);
                        *((edict_t**)pOutputData) = pent;
                        break;
                    case FIELD_EHANDLE:
                        // Input and Output sizes are different!
                        pOutputData = (char*)pOutputData + j * (sizeof(EHANDLE) - gSizes[pTest->fieldType]);
                        entityIndex = *(int*)pInputData;
                        pent = EntityFromIndex(entityIndex);
                        if(pent)
                            *((EHANDLE*)pOutputData) = CBaseEntity::Instance(pent);
                        else
                            *((EHANDLE*)pOutputData) = NULL;
                        break;
                    case FIELD_ENTITY:
                        entityIndex = *(int*)pInputData;
                        pent = EntityFromIndex(entityIndex);
                        if(pent)
                            *((EOFFSET*)pOutputData) = OFFSET(pent);
                        else
                            *((EOFFSET*)pOutputData) = 0;
                        break;
                    case FIELD_VECTOR:
                        ((float*)pOutputData)[0] = ((float*)pInputData)[0];
                        ((float*)pOutputData)[1] = ((float*)pInputData)[1];
                        ((float*)pOutputData)[2] = ((float*)pInputData)[2];
                        break;
                    case FIELD_POSITION_VECTOR:
                        ((float*)pOutputData)[0] = ((float*)pInputData)[0] + position.x;
                        ((float*)pOutputData)[1] = ((float*)pInputData)[1] + position.y;
                        ((float*)pOutputData)[2] = ((float*)pInputData)[2] + position.z;
                        break;

                    case FIELD_BOOLEAN:
                    case FIELD_INTEGER:
                        *((int*)pOutputData) = *(int*)pInputData;
                        break;

                    case FIELD_SHORT:
                        *((short*)pOutputData) = *(short*)pInputData;
                        break;

                    case FIELD_CHARACTER:
                        *((char*)pOutputData) = *(char*)pInputData;
                        break;

                    case FIELD_POINTER:
                        *((int*)pOutputData) = *(int*)pInputData;
                        break;
                    case FIELD_FUNCTION:
                        if(strlen((char*)pInputData) == 0)
                            *((int*)pOutputData) = 0;
                        else
                            *((int*)pOutputData) = FUNCTION_FROM_NAME((char*)pInputData);
                        break;

                    default:
                        ALERT(at_error, "Bad field type\n");
                    }
                }
            }
#if 0
			else
			{
				ALERT( at_console, "Skipping global field %s\n", pName );
			}
#endif
            return fieldNumber;
        }
    }

    return -1;
}

int CRestore::ReadEntVars(const char* pname, entvars_t* pev)
{
    return ReadFields(pname, pev, gEntvarsDescription, ENTVARS_COUNT);
}

int CRestore::ReadFields(const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount)
{
    unsigned short i, token;
    int lastField, fileCount;
    HEADER header;

    i = ReadShort();
    // ASSERT( i == sizeof(int) );			// First entry should be an int

    token = ReadShort();

    // Check the struct name
    if(token != TokenHash(pname)) // Field Set marker
    {
        //		ALERT( at_error, "Expected %s found %s!\n", pname, BufferPointer() );
        BufferRewind(2 * sizeof(short));
        return 0;
    }

    // Skip over the struct name
    fileCount = ReadInt(); // Read field count

    lastField = 0; // Make searches faster, most data is read/written in the same order

    // Clear out base data
    for(i = 0; i < fieldCount; i++) {
        // Don't clear global fields
        if(!m_global || !(pFields[i].flags & FTYPEDESC_GLOBAL))
            memset(((char*)pBaseData + pFields[i].fieldOffset), 0, pFields[i].fieldSize * gSizes[pFields[i].fieldType]);
    }

    for(i = 0; i < fileCount; i++) {
        BufferReadHeader(&header);
        lastField = ReadField(
            pBaseData, pFields, fieldCount, lastField, header.size, m_pdata->pTokens[header.token], header.pData);
        lastField++;
    }

    return 1;
}

void CRestore::BufferReadHeader(HEADER* pheader)
{
    // ASSERT( pheader!=NULL );
    pheader->size = ReadShort();      // Read field size
    pheader->token = ReadShort();     // Read field name token
    pheader->pData = BufferPointer(); // Field Data is next
    BufferSkipBytes(pheader->size);   // Advance to next field
}

short CRestore::ReadShort(void)
{
    short tmp = 0;

    BufferReadBytes((char*)&tmp, sizeof(short));

    return tmp;
}

int CRestore::ReadInt(void)
{
    int tmp = 0;

    BufferReadBytes((char*)&tmp, sizeof(int));

    return tmp;
}

void CRestore::BufferReadBytes(char* pOutput, int size)
{
    // ASSERT( m_pdata !=NULL );

    if(!m_pdata || Empty())
        return;

    if((m_pdata->size + size) > m_pdata->bufferSize) {
        ALERT(at_error, "Restore overflow!");
        m_pdata->size = m_pdata->bufferSize;
        return;
    }

    if(pOutput)
        memcpy(pOutput, m_pdata->pCurrentData, size);
    m_pdata->pCurrentData += size;
    m_pdata->size += size;
}

void CRestore::BufferSkipBytes(int bytes)
{
    BufferReadBytes(NULL, bytes);
}

char* CRestore::BufferPointer(void)
{
    if(!m_pdata)
        return NULL;

    return m_pdata->pCurrentData;
}

// extern Vector VecBModelOrigin( entvars_t* pevBModel );
extern DLL_GLOBAL Vector g_vecAttackDir;
extern DLL_GLOBAL int g_iSkillLevel;

static void SetObjectCollisionBox(entvars_t* pev);

/*#ifndef _WIN32
extern "C" {
#endif
int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
{
        if ( !pFunctionTable || interfaceVersion != INTERFACE_VERSION )
        {
                return FALSE;
        }

        memcpy( pFunctionTable, &gFunctionTable, sizeof( DLL_FUNCTIONS ) );
        return TRUE;
}*/

#ifndef _WIN32
}
#endif

edict_t* EHANDLE::Get(void)
{
    if(m_pent) {
        if(m_pent->serialnumber == m_serialnumber)
            return m_pent;
        else
            return NULL;
    }
    return NULL;
};

edict_t* EHANDLE::Set(edict_t* pent)
{
    m_pent = pent;
    if(pent)
        m_serialnumber = m_pent->serialnumber;
    return pent;
};

EHANDLE::operator CBaseEntity*()
{
    return (CBaseEntity*)GET_PRIVATE(Get());
};

CBaseEntity* EHANDLE::operator=(CBaseEntity* pEntity)
{
    if(pEntity) {
        m_pent = ENT(pEntity->pev);
        if(m_pent)
            m_serialnumber = m_pent->serialnumber;
    } else {
        m_pent = NULL;
        m_serialnumber = 0;
    }
    return pEntity;
}

EHANDLE::operator int()
{
    return Get() != NULL;
}

CBaseEntity* EHANDLE::operator->()
{
    return (CBaseEntity*)GET_PRIVATE(Get());
}

// give health
int CBaseEntity::TakeHealth(float flHealth, int bitsDamageType)
{
    if(!pev->takedamage)
        return 0;

    // heal
    if(pev->health >= pev->max_health)
        return 0;

    pev->health += flHealth;

    if(pev->health > pev->max_health)
        pev->health = pev->max_health;

    return 1;
}

// inflict damage on this entity.  bitsDamageType indicates type of damage inflicted, ie: DMG_CRUSH

int CBaseEntity::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
    Vector vecTemp;

    if(!pev->takedamage)
        return 0;

    // UNDONE: some entity types may be immune or resistant to some bitsDamageType

    // if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
    // (that is, no actual entity projectile was involved in the attack so use the shooter's origin).
    if(pevAttacker == pevInflictor) {
        vecTemp = pevInflictor->origin - (VecBModelOrigin(edict()));
    } else
    // an actual missile was involved.
    {
        vecTemp = pevInflictor->origin - (VecBModelOrigin(edict()));
    }

    // this global is still used for glass and other non-monster killables, along with decals.
    g_vecAttackDir = vecTemp.Normalize();

    // save damage based on the target's armor level

    // figure momentum add (don't let hurt brushes or other triggers move player)
    if((!FNullEnt(pevInflictor)) && (pev->movetype == MOVETYPE_WALK || pev->movetype == MOVETYPE_STEP) &&
        (pevAttacker->solid != SOLID_TRIGGER)) {
        Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
        vecDir = vecDir.Normalize();

        float flForce = flDamage * ((32 * 32 * 72.0) / (pev->size.x * pev->size.y * pev->size.z)) * 5;

        if(flForce > 1000.0)
            flForce = 1000.0;
        pev->velocity = pev->velocity + vecDir * flForce;
    }

    // do the damage
    pev->health -= flDamage;
    if(pev->health <= 0) {
        Killed(pevAttacker, GIB_NORMAL);
        return 0;
    }

    return 1;
}

void CBaseEntity::Killed(entvars_t* pevAttacker, int iGib)
{
    pev->takedamage = DAMAGE_NO;
    pev->deadflag = DEAD_DEAD;
    UTIL_Remove(this);
}

CBaseEntity* CBaseEntity::GetNextTarget(void)
{
    if(FStringNull(pev->target))
        return NULL;
    edict_t* pTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));
    if(FNullEnt(pTarget))
        return NULL;

    return Instance(pTarget);
}

// Global Savedata for Delay
TYPEDESCRIPTION CBaseEntity::m_SaveData[] = {
    DEFINE_FIELD(CBaseEntity, m_pGoalEnt, FIELD_CLASSPTR),

    DEFINE_FIELD(CBaseEntity, m_pfnThink, FIELD_FUNCTION), // UNDONE: Build table of these!!!
    DEFINE_FIELD(CBaseEntity, m_pfnTouch, FIELD_FUNCTION), DEFINE_FIELD(CBaseEntity, m_pfnUse, FIELD_FUNCTION),
    DEFINE_FIELD(CBaseEntity, m_pfnBlocked, FIELD_FUNCTION),
};

int CBaseEntity::Save(CSave& save)
{
    if(save.WriteEntVars("ENTVARS", pev))
        return save.WriteFields("BASE", this, m_SaveData, ARRAYSIZE(m_SaveData));

    return 0;
}

int CBaseEntity::Restore(CRestore& restore)
{
    int status;

    status = restore.ReadEntVars("ENTVARS", pev);
    if(status)
        status = restore.ReadFields("BASE", this, m_SaveData, ARRAYSIZE(m_SaveData));

    if(pev->modelindex != 0 && !FStringNull(pev->model)) {
        Vector mins, maxs;
        mins = pev->mins; // Set model is about to destroy these
        maxs = pev->maxs;

        PRECACHE_MODEL((char*)STRING(pev->model));
        SET_MODEL(ENT(pev), STRING(pev->model));
        UTIL_SetSize(pev, mins, maxs); // Reset them
    }

    return status;
}

// Initialize absmin & absmax to the appropriate box
void SetObjectCollisionBox(entvars_t* pev)
{
    if((pev->solid == SOLID_BSP) && (pev->angles.x || pev->angles.y || pev->angles.z)) { // expand for rotation
        float max, v;
        int i;

        max = 0;
        for(i = 0; i < 3; i++) {
            v = fabs(((float*)pev->mins)[i]);
            if(v > max)
                max = v;
            v = fabs(((float*)pev->maxs)[i]);
            if(v > max)
                max = v;
        }
        for(i = 0; i < 3; i++) {
            ((float*)pev->absmin)[i] = ((float*)pev->origin)[i] - max;
            ((float*)pev->absmax)[i] = ((float*)pev->origin)[i] + max;
        }
    } else {
        pev->absmin = pev->origin + pev->mins;
        pev->absmax = pev->origin + pev->maxs;
    }

    pev->absmin.x -= 1;
    pev->absmin.y -= 1;
    pev->absmin.z -= 1;
    pev->absmax.x += 1;
    pev->absmax.y += 1;
    pev->absmax.z += 1;
}

void CBaseEntity::SetObjectCollisionBox(void)
{
    ::SetObjectCollisionBox(pev);
}

int CBaseEntity::Intersects(CBaseEntity* pOther)
{
    if(pOther->pev->absmin.x > pev->absmax.x || pOther->pev->absmin.y > pev->absmax.y ||
        pOther->pev->absmin.z > pev->absmax.z || pOther->pev->absmax.x < pev->absmin.x ||
        pOther->pev->absmax.y < pev->absmin.y || pOther->pev->absmax.z < pev->absmin.z)
        return 0;
    return 1;
}

void CBaseEntity::MakeDormant(void)
{
    SetBits(pev->flags, FL_DORMANT);

    // Don't touch
    pev->solid = SOLID_NOT;
    // Don't move
    pev->movetype = MOVETYPE_NONE;
    // Don't draw
    SetBits(pev->effects, EF_NODRAW);
    // Don't think
    pev->nextthink = 0;
    // Relink
    UTIL_SetOrigin(pev, pev->origin);
}

int CBaseEntity::IsDormant(void)
{
    return FBitSet(pev->flags, FL_DORMANT);
}

BOOL CBaseEntity::IsInWorld(void)
{
    // position
    if(pev->origin.x >= 4096)
        return FALSE;
    if(pev->origin.y >= 4096)
        return FALSE;
    if(pev->origin.z >= 4096)
        return FALSE;
    if(pev->origin.x <= -4096)
        return FALSE;
    if(pev->origin.y <= -4096)
        return FALSE;
    if(pev->origin.z <= -4096)
        return FALSE;
    // speed
    if(pev->velocity.x >= 2000)
        return FALSE;
    if(pev->velocity.y >= 2000)
        return FALSE;
    if(pev->velocity.z >= 2000)
        return FALSE;
    if(pev->velocity.x <= -2000)
        return FALSE;
    if(pev->velocity.y <= -2000)
        return FALSE;
    if(pev->velocity.z <= -2000)
        return FALSE;

    return TRUE;
}

int CBaseEntity::ShouldToggle(USE_TYPE useType, BOOL currentState)
{
    if(useType != USE_TOGGLE && useType != USE_SET) {
        if((currentState && useType == USE_ON) || (!currentState && useType == USE_OFF))
            return 0;
    }
    return 1;
}

int CBaseEntity::DamageDecal(int bitsDamageType)
{
    if(pev->rendermode == kRenderTransAlpha)
        return -1;

    if(pev->rendermode != kRenderNormal)
        return DECAL_BPROOF1;

    return DECAL_GUNSHOT1 + RANDOM_LONG(0, 4);
}

// NOTE: szName must be a pointer to constant memory, e.g. "monster_class" because the entity
// will keep a pointer to it after this call.
CBaseEntity* CBaseEntity::Create(char* szName, const Vector& vecOrigin, const Vector& vecAngles, edict_t* pentOwner)
{
    edict_t* pent;
    CBaseEntity* pEntity;

    pent = CREATE_NAMED_ENTITY(MAKE_STRING(szName));
    if(FNullEnt(pent)) {
        ALERT(at_console, "NULL Ent in Create!\n");
        return NULL;
    }
    pEntity = Instance(pent);
    pEntity->pev->owner = pentOwner;
    pEntity->pev->origin = vecOrigin;
    pEntity->pev->angles = vecAngles;
    DispatchSpawn(pEntity->edict());
    return pEntity;
}
