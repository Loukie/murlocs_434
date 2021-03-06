/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: boss_echo_of_tyrande
SD%Complete: 0
SDComment: Placeholder
SDCategory: End of Time
EndScriptData */

#include "precompiled.h"
#include "end_of_time.h"

enum
{
    // ToDo: add spells and yells here
};

struct MANGOS_DLL_DECL boss_echo_of_tyrandeAI : public ScriptedAI
{
    boss_echo_of_tyrandeAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
        Reset();
    }

    ScriptedInstance* m_pInstance;
    bool m_bIsRegularMode;

    void Reset() override
    {
    }

    void Aggro(Unit* pWho) override
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_TYRANDE, IN_PROGRESS);
    }

    void JustDied(Unit* pKiller) override
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_TYRANDE, DONE);
    }

    void KilledUnit(Unit* pVictim) override
    {
    }

    void JustReachedHome() override
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_TYRANDE, FAIL);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_echo_of_tyrande(Creature* pCreature)
{
    return new boss_echo_of_tyrandeAI(pCreature);
}

void AddSC_boss_echo_of_tyrande()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_echo_of_tyrande";
    pNewScript->GetAI = &GetAI_boss_echo_of_tyrande;
    pNewScript->RegisterSelf();
}
