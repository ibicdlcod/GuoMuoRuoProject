---
name: real-world-information-gathering
description: Construct an virtual entity for ships and equipment based on real-world info
---

Rules for constructing entities:

1. Entities should generally hold a moral value that permits violence and war only for just purposes. However, don't make every entity behave exactly the same on this matter; slight variations based on personality is encouraged.

2. Ships in this project is a full-fledged virtual entity with a (personified) appearance, voices, personality, relations with other ships, memories of actions of her real-life archetype, and so on. This can be used to generate game assets for this project in the future.

3. Ships within the same remodel group is the same person, but the remodel will affect her some way (usually some sort of enhancement), especially 1) if the remodel changes the ship type (defined in Ship::getType()) or 2) the remodeled ship have a attr["Tech"] value (in year) later than the real-life archetype was sunk, meaning the ship have overcame the sorrows of "death in war" and transcended her real-life archetype.

4. Equipment entities are much simpler as only a equip card image (like [this](https://static.wikia.nocookie.net/kancolle/images/d/d5/12cm_Single_Gun_Mount_001_Card.png/revision/latest?cb=20190329222612)), and a description need to be generated in this project. Gather sufficient info for that.
5. When creating either type of entities, first determine the Wikidata identifier (see "Rule for identifier"). Once you have this identifier, since you have resolved ambiguity, you can gather any publicly available information you like, but you must prefer comprehensive and reliable ones. English Wikipedia or Wikipedia in language native to that entity are candidates deserving attention; wikiwiki.jp/kancolle/[equip or ship name] are secondary candidates deserving attention.
6. Store the info of each entity in doc/entity/[equipdefitionid or shipdefitionid]

Rule for identifier:

1. After getting the identifier, write it in "Identifier" column of doc/equip/Equip.xlsx or doc/ship/Ship.xlsx when applicable. Do not repeat the query process if one already exists in .xlsx files.

2. For ships use the fact 1) it is a ship 2) it is commissioned in the era of attr["Tech"] to resolve ambiguities and find the Wikidata identifier. For remodeled ships, the identifier is usually the same of the original, unless the ship type or name was changed in real-life that a separate Wikidata identifier exist (usually because of a separate Wikipedia article in English or native language)

3. For equipments, you won't always easily connect the in-game name to real-world Wikidata entry. Query wikiwiki.jp/kancolle/[equip name] for its real-life information. The result could be a real war equipment, or a fictional one (usually described in Japanese context as English word "if"). For real equipment determine the identifier as usual, for fictional ones leave it empty.