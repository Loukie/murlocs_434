/* Copyright (C) 2006 - 2012 ScriptDev2 <http://www.scriptdev2.com/>
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
SDName: Boss_Terestian_Illhoof
SD%Complete: 95
SDComment: Complete! Needs adjustments to use spell though.
SDCategory: Karazhan
EndScriptData */

#include "precompiled.h"
#include "karazhan.h"

enum
{
    SAY_SLAY1                   = -1532065,
    SAY_SLAY2                   = -1532066,
    SAY_DEATH                   = -1532067,
    SAY_AGGRO                   = -1532068,
    SAY_SACRIFICE1              = -1532069,
    SAY_SACRIFICE2              = -1532070,
    SAY_SUMMON1                 = -1532071,
    SAY_SUMMON2                 = -1532072,

    SPELL_SUMMON_DEMONCHAINS    = 30120,                    // Summons demonic chains that maintain the ritual of sacrifice.
    SPELL_DEMON_CHAINS          = 30206,                    // Instant - Visual Effect
    SPELL_ENRAGE                = 23537,                    // Increases the caster's attack speed by 50% and the Physical damage it deals by 219 to 281 for 10 min.
    SPELL_SHADOW_BOLT           = 30055,                    // Hurls a bolt of dark magic at an enemy, inflicting Shadow damage.
    SPELL_SACRIFICE             = 30115,                    // Teleports and adds the debuff
    SPELL_BERSERK               = 32965,                    // Increases attack speed by 75%. Periodically casts Shadow Bolt Volley.

    SPELL_SUMMON_IMP            = 30066,                    // Summons Kil'rek

    SPELL_SUMMON_FIENDISH_IMP   = 30184,
    SPELL_FIENDISH_PORTAL       = 30171,                    // Opens portal and summons Fiendish Portal, 2 sec cast
    SPELL_FIENDISH_PORTAL_1     = 30179,                    // Opens portal and summons Fiendish Portal, instant cast

    SPELL_FIREBOLT              = 30050,                    // Blasts a target for 150 Fire damage.

    SPELL_BROKEN_PACT           = 30065,                    // All damage taken increased by 25%.
    SPELL_AMPLIFY_FLAMES        = 30053,                    // Increases the Fire damage taken by an enemy by 500 for 25 sec.

    NPC_DEMONCHAINS             = 17248,
    NPC_FIENDISHIMP             = 17267,
    NPC_PORTAL                  = 17265,
    NPC_KILREK                  = 17229
};

struct MANGOS_DLL_DECL mob_demon_chainAI : public ScriptedAI
{
    mob_demon_chainAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    ObjectGuid m_sacrificeGuid;

    void Reset(){}

    void AttackStart(Unit* pWho) {}
    void MoveInLineOfSight(Unit* pWho) {}

    void JustDied(Unit* pKiller)
    {
        if (m_sacrificeGuid)
        {
            if (Player* pSacrifice = m_creature->GetMap()->GetPlayer(m_sacrificeGuid))
                pSacrifice->RemoveAurasDueToSpell(SPELL_SACRIFICE);
        }
    }
};

struct MANGOS_DLL_DECL boss_terestianAI : public ScriptedAI
{
    boss_terestianAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        m_bSummonKilrek = true;
        Reset();
    }

    ScriptedInstance* m_pInstance;

    ObjectGuid m_aPortalGuid[2];

    uint32 m_uiSummonKilrekTimer;
    uint32 m_uiSacrifice_Timer;
    uint32 m_uiShadowbolt_Timer;
    uint32 m_uiSummon_Timer;
    uint32 m_uiBerserk_Timer;

    bool m_bSummonKilrek;
    bool m_bSummonedPortals;
    bool m_bBerserk;

    void Reset()
    {
        m_uiSummonKilrekTimer   = 5000;
        m_uiSacrifice_Timer     = 30000;
        m_uiShadowbolt_Timer    = 5000;
        m_uiSummon_Timer        = 10000;
        m_uiBerserk_Timer       = 600000;

        m_bSummonedPortals      = false;
        m_bBerserk              = false;

        if (!m_pInstance)
            return;

        for(uint8 i = 0; i < 2; ++i)
        {
            if (m_aPortalGuid[i])
            {
                if (Creature* pPortal = m_pInstance->instance->GetCreature(m_aPortalGuid[i]))
                    pPortal->ForcedDespawn();

                m_aPortalGuid[i].Clear();
            }
        }

        if (!m_creature->isAlive())
            return;

        m_pInstance->SetData(TYPE_TERESTIAN, NOT_STARTED);

        if (!m_creature->GetPet())
            m_bSummonKilrek = true;
    }

    void Aggro(Unit* pWho)
    {
        if (Pet* pKilrek = m_creature->GetPet())
            pKilrek->SetInCombatWithZone();

        DoScriptText(SAY_AGGRO, m_creature);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_TERESTIAN, IN_PROGRESS);
    }

    void KilledUnit(Unit* pVictim)
    {
        DoScriptText(urand(0, 1) ? SAY_SLAY1 : SAY_SLAY2, m_creature);
    }

    void JustSummoned(Creature* pSummoned)
    {
        switch(pSummoned->GetEntry())
        {
            case NPC_PORTAL:
            {
                if (m_aPortalGuid[0])
                {
                    m_aPortalGuid[1] = pSummoned->GetObjectGuid();

                    if (npc_fiendish_portalAI* pPortalAI = dynamic_cast<npc_fiendish_portalAI*>(pSummoned->AI()))
                        pPortalAI->m_uiSummonTimer = 10000;
                }
                else
                {
                    m_aPortalGuid[0] = pSummoned->GetObjectGuid();
                    DoCastSpellIfCan(m_creature, SPELL_FIENDISH_PORTAL_1, CAST_TRIGGERED);
                }

                break;
            }
            case NPC_KILREK:
                m_creature->RemoveAurasDueToSpell(SPELL_BROKEN_PACT);
                break;
        }
    }

    void SummonedCreatureJustDied(Creature* pSummoned)
    {
        if (pSummoned->GetEntry() == NPC_KILREK)
        {
            DoCastSpellIfCan(m_creature, SPELL_BROKEN_PACT, CAST_TRIGGERED);
            m_bSummonKilrek = true;
        }
    }

    void JustDied(Unit* pKiller)
    {
        DoScriptText(SAY_DEATH, m_creature);

        if (!m_pInstance)
            return;

        for(uint8 i = 0; i < 2; ++i)
        {
            if (m_aPortalGuid[i])
            {
                if (Creature* pPortal = m_pInstance->instance->GetCreature(m_aPortalGuid[i]))
                    pPortal->ForcedDespawn();

                m_aPortalGuid[i].Clear();
            }
        }

        m_pInstance->SetData(TYPE_TERESTIAN, DONE);
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (m_bSummonKilrek)
        {
            if (m_uiSummonKilrekTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_SUMMON_IMP) == CAST_OK)
                {
                    m_uiSummonKilrekTimer = 45000;
                    m_bSummonKilrek = false;
                }
            }
            else
                m_uiSummonKilrekTimer -= uiDiff;
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiSacrifice_Timer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1, SPELL_SACRIFICE, SELECT_FLAG_PLAYER))
            {
                DoCastSpellIfCan(pTarget, SPELL_SACRIFICE, CAST_TRIGGERED);

                if (Creature* pChains = m_creature->SummonCreature(NPC_DEMONCHAINS, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), 0, TEMPSUMMON_TIMED_OOC_OR_CORPSE_DESPAWN, 21000))
                {
                    if (mob_demon_chainAI* pDemonAI = dynamic_cast<mob_demon_chainAI*>(pChains->AI()))
                        pDemonAI->m_sacrificeGuid = pTarget->GetObjectGuid();

                    pChains->CastSpell(pChains, SPELL_DEMON_CHAINS, true);

                    DoScriptText(urand(0, 1) ? SAY_SACRIFICE1 : SAY_SACRIFICE2, m_creature);

                    m_uiSacrifice_Timer = 30000;
                }
            }
        }
        else
            m_uiSacrifice_Timer -= uiDiff;

        if (m_uiShadowbolt_Timer < uiDiff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_SHADOW_BOLT);
            m_uiShadowbolt_Timer = 10000;
        }
        else
            m_uiShadowbolt_Timer -= uiDiff;

        if (!m_bSummonedPortals)
        {
            if (m_uiSummon_Timer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_FIENDISH_PORTAL, CAST_INTERRUPT_PREVIOUS) == CAST_OK)
                {
                    DoScriptText(urand(0, 1) ? SAY_SUMMON1 : SAY_SUMMON2, m_creature);
                    m_bSummonedPortals = true;
                }
            }
            else
                m_uiSummon_Timer -= uiDiff;
        }

        if (!m_bBerserk)
        {
            if (m_uiBerserk_Timer < uiDiff)
            {
                DoCastSpellIfCan(m_creature, SPELL_BERSERK, CAST_INTERRUPT_PREVIOUS);
                m_bBerserk = true;
            }
            else
                m_uiBerserk_Timer -= uiDiff;
        }

        DoMeleeAttackIfReady();
    }
};

npc_fiendish_portalAI::npc_fiendish_portalAI(Creature* pCreature) : ScriptedAI(pCreature),
    m_uiSummonTimer(5000)
{
    Reset();
}

void npc_fiendish_portalAI::Reset()
{
}

void npc_fiendish_portalAI::JustSummoned(Creature* pSummoned)
{
    pSummoned->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FIRE, true);
    pSummoned->SetInCombatWithZone();
}

void npc_fiendish_portalAI::UpdateAI(const uint32 uiDiff)
{
    if (m_uiSummonTimer < uiDiff)
    {
        DoCastSpellIfCan(m_creature, SPELL_SUMMON_FIENDISH_IMP);
        m_uiSummonTimer = 10000;
    }
    else
        m_uiSummonTimer -= uiDiff;
}

CreatureAI* GetAI_npc_fiendish_portal(Creature* pCreature)
{
    return new npc_fiendish_portalAI(pCreature);
}

CreatureAI* GetAI_mob_demon_chain(Creature* pCreature)
{
    return new mob_demon_chainAI(pCreature);
}

CreatureAI* GetAI_boss_terestian_illhoof(Creature* pCreature)
{
    return new boss_terestianAI(pCreature);
}

void AddSC_boss_terestian_illhoof()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_terestian_illhoof";
    pNewScript->GetAI = &GetAI_boss_terestian_illhoof;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_fiendish_portal";
    pNewScript->GetAI = &GetAI_npc_fiendish_portal;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "mob_demon_chain";
    pNewScript->GetAI = &GetAI_mob_demon_chain;
    pNewScript->RegisterSelf();
}
