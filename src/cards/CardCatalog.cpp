#include "cards/CardCatalog.h"
#include <algorithm>
#include <stdexcept>

namespace fl {
std::vector<CardDefinition> unlimitedDefinitions() {
    return {
#include "cards/UnlimitedCards.inc"
    };
}
std::vector<std::size_t> unlimitedDemoDeck(const std::vector<CardDefinition>& definitions,int side) {
    const std::string colors=side==0 ? "UG":"RW";
    const std::string first=side==0 ? "Island":"Mountain",second=side==0 ? "Forest":"Plains";
    std::vector<std::size_t> firstLands,secondLands,spells,deck;
    for(std::size_t i=0;i<definitions.size();++i) {
        const auto& d=definitions[i];
        if(d.basicLand && d.name==first) firstLands.push_back(i);
        if(d.basicLand && d.name==second) secondLands.push_back(i);
        const bool matching=std::all_of(d.colorIdentity.begin(),d.colorIdentity.end(),[&](char c){return colors.find(c)!=std::string::npos;});
        if(!d.basicLand && d.type.find("Land")==std::string::npos && matching) spells.push_back(i);
    }
    if(firstLands.empty() || secondLands.empty() || spells.empty()) throw std::runtime_error("Unlimited demo deck data incomplete");
    for(std::size_t i=0;i<12;++i) {deck.push_back(firstLands[i%firstLands.size()]);deck.push_back(secondLands[i%secondLands.size()]);}
    for(std::size_t i=0;i<36;++i) deck.push_back(spells[i%spells.size()]);
    return deck; // Test lists, not format-legality enforcement or saved collections.
}
}
