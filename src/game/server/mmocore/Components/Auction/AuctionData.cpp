/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "AuctionData.h"

#include <engine/shared/config.h>

int CAuctionSlot::GetTaxPrice() const
{
	const int TaxPrice = max(1, translate_to_percent_rest(m_Price, g_Config.m_SvAuctionSlotTaxPrice));
	return TaxPrice;
}
