#include "PartyDamage.h"

#include <sstream>

#include <GWCA\GWCA.h>
#include <GWCA\Managers\ChatMgr.h>
#include <GWCA\Managers\StoCMgr.h>
#include <GWCA\Managers\PartyMgr.h>

#include <OSHGui\OSHGui.hpp>

#include "GuiUtils.h"
#include "Config.h"

using namespace OSHGui;

PartyDamage::PartyDamage() {

	total = 0;
	send_timer = TBTimer::init();

	GW::StoC().AddGameServerEvent<GW::Packet::StoC::P151>(
		std::bind(&PartyDamage::DamagePacketCallback, this, std::placeholders::_1));

	GW::StoC().AddGameServerEvent<GW::Packet::StoC::P230>(
		std::bind(&PartyDamage::MapLoadedCallback, this, std::placeholders::_1));

	int x = Config::IniReadLong(PartyDamage::IniSection(), PartyDamage::IniKeyX(), 400);
	int y = Config::IniReadLong(PartyDamage::IniSection(), PartyDamage::IniKeyY(), 100);
	SetLocation(PointI(x, y));

	line_height_ = GuiUtils::GetPartyHealthbarHeight();
	SetSize(OSHGui::Drawing::SizeI(ABS_WIDTH + PERC_WIDTH, line_height_ * MAX_PLAYERS));

	if (!Application::Instance().GetTheme().ContainsColorTheme(PartyDamage::ThemeKey())) {
		Drawing::Theme::ControlTheme ctheme(default_forecolor, default_backcolor);
		Drawing::Theme::ControlTheme barctheme(default_forebarcolor, default_backbarcolor);
		Drawing::Theme& theme = Application::Instance().GetTheme();
		theme.SetControlColorTheme(PartyDamage::ThemeKey(), ctheme);
		theme.SetControlColorTheme(PartyDamage::ThemeBarsKey(), barctheme);
	}

	Drawing::Theme::ControlTheme theme = Application::Instance()
		.GetTheme().GetControlColorTheme(PartyDamage::ThemeKey());
	Drawing::Theme::ControlTheme bartheme = Application::Instance()
		.GetTheme().GetControlColorTheme(PartyDamage::ThemeBarsKey());

	SetTransparentBackColor(false);
	labelcolor = theme.ForeColor;

	int offsetX = 2;
	int offsetY = 2;
	float fontsize = 9.0f;
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		damage[i].damage= 0;
		damage[i].recent_damage = 0;
		damage[i].last_damage = TBTimer::init();

		absolute[i] = new DragButton(containerPanel_);
		absolute[i]->SetText(L"0 %");
		absolute[i]->SetSize(SizeI(ABS_WIDTH, line_height_ - RECENT_HEIGHT / 2));
		absolute[i]->SetLocation(PointI(0, i * line_height_));
		absolute[i]->SetFont(GuiUtils::getTBFont(fontsize, true));
		absolute[i]->SetBackColor(Drawing::Color::Empty());
		absolute[i]->SetForeColor(theme.ForeColor);
		absolute[i]->GetMouseUpEvent() += MouseUpEventHandler([this](Control*, MouseEventArgs) {
			SaveLocation();
		});
		AddControl(absolute[i]);

		percent[i] = new DragButton(containerPanel_);
		percent[i]->SetText(L"");
		percent[i]->SetSize(SizeI(PERC_WIDTH, line_height_ - RECENT_HEIGHT / 2));
		percent[i]->SetLocation(PointI(ABS_WIDTH, i * line_height_));
		percent[i]->SetFont(GuiUtils::getTBFont(fontsize, true));
		percent[i]->SetBackColor(Drawing::Color::Empty());
		percent[i]->SetForeColor(theme.ForeColor);
		percent[i]->GetMouseUpEvent() += MouseUpEventHandler([this](Control*, MouseEventArgs) {
			SaveLocation();
		});
		AddControl(percent[i]);

		recent[i] = new Panel(containerPanel_);
		recent[i]->SetSize(SizeI(WIDTH, RECENT_HEIGHT));
		recent[i]->SetLocation(PointI(0, (i + 1) * line_height_ - RECENT_HEIGHT));
		recent[i]->SetBackColor(bartheme.ForeColor);
		AddControl(recent[i]);

		bar[i] = new Panel(containerPanel_);
		bar[i]->SetSize(SizeI(WIDTH, line_height_));
		bar[i]->SetLocation(PointI(0, i * line_height_));
		bar[i]->SetBackColor(bartheme.BackColor);
		AddControl(bar[i]);
	}

	std::shared_ptr<PartyDamage> self = std::shared_ptr<PartyDamage>(this);
	Form::Show(self);

	bool show = Config::IniReadBool(PartyDamage::IniSection(), PartyDamage::InikeyShow(), false);
	SetVisible(show);

	LoadIni();
}

PartyDamage::~PartyDamage() {
	inifile_->Reset();
	delete inifile_;
}

bool PartyDamage::MapLoadedCallback(GW::Packet::StoC::P230* packet) {
	switch (GW::Map().GetInstanceType()) {
	case GW::Constants::InstanceType::Outpost:
		in_explorable = false;
		break;
	case GW::Constants::InstanceType::Explorable:
		party_index.clear();
		if (!in_explorable) {
			in_explorable = true;
			ResetDamage();
		}
		break;
	case GW::Constants::InstanceType::Loading:
	default:
		break;
	}
	return false;
}

bool PartyDamage::DamagePacketCallback(GW::Packet::StoC::P151* packet) {

	// ignore non-damage packets
	switch (packet->type) {
	case GW::Packet::StoC::P151_Type::damage:
	case GW::Packet::StoC::P151_Type::critical:
	case GW::Packet::StoC::P151_Type::armorignoring:
		break;
	default:
		return false;
	}

	// ignore heals
	if (packet->value >= 0) return false;

	GW::AgentArray agents = GW::Agents().GetAgentArray();

	// get cause agent
	GW::Agent* cause = agents[packet->cause_id];
	
	if (cause == nullptr) return false;
	if (cause->Allegiance != 0x1) return false;
	auto cause_it = party_index.find(cause->Id);
	if (cause_it == party_index.end()) return false;  // ignore damage done by non-party members

	// get target agent
	GW::Agent* target = agents[packet->target_id];
	if (target == nullptr) return false;
	if (target->LoginNumber != 0) return false; // ignore player-inflicted damage
										        // such as Life bond or sacrifice
	if (target->Allegiance == 0x1) return false; // ignore damage inflicted to allies in general
	// warning: note damage to allied spirits, minions or stones may still trigger
	// you can do damage like that by standing in bugged dart traps in eye of the north
	// or maybe with some skills that damage minions/spirits

	long dmg;
	if (target->MaxHP > 0 && target->MaxHP < 100000) {
		dmg = std::lround(-packet->value * target->MaxHP);
		hp_map[target->PlayerNumber] = target->MaxHP;
	} else {
		auto it = hp_map.find(target->PlayerNumber);
		if (it == hp_map.end()) {
			// max hp not found, approximate with hp/lvl formula
			dmg = std::lround(-packet->value * (target->Level * 20 + 100));
		} else {
			long maxhp = it->second;
			dmg = std::lround(-packet->value * it->second);
		}
	}

	int index = cause_it->second;
	if (index >= MAX_PLAYERS) return false; // something went very wrong.
	if (damage[index].damage == 0) {
		damage[index].agent_id = packet->cause_id;
		if (cause->LoginNumber > 0) {
			damage[index].name = GW::Agents().GetPlayerNameByLoginNumber(cause->LoginNumber);
		} else {
			damage[index].name = L"<A Hero>";
		}
		damage[index].primary = static_cast<GW::Constants::Profession>(cause->Primary);
		damage[index].secondary = static_cast<GW::Constants::Profession>(cause->Secondary);
	}

	damage[index].damage += dmg;
	total += dmg;

	if (isVisible_) {
		damage[index].recent_damage += dmg;
		damage[index].last_damage = TBTimer::init();
	}
	return false;
}

void PartyDamage::Main() {
	if (!send_queue.empty() && TBTimer::diff(send_timer) > 600) {
		send_timer = TBTimer::init();
		if (GW::Map().GetInstanceType() != GW::Constants::InstanceType::Loading
			&& GW::Agents().GetPlayer()) {
			GW::Chat().SendChat(send_queue.front().c_str(), L'#');
			send_queue.pop();
		}
	}

	if (party_index.empty()) {
		CreatePartyIndexMap();
	}
}

void PartyDamage::CreatePartyIndexMap() {
	if (!GW::Partymgr().GetIsPartyLoaded()) return;
	
	GW::PartyInfo* info = GW::Partymgr().GetPartyInfo();
	if (info == nullptr) return;

	GW::PlayerArray players = GW::Agents().GetPlayerArray();
	if (!players.valid()) return;

	int index = 0;
	for (GW::PlayerPartyMember player : info->players) {
		long id = players[player.loginnumber].AgentID;
		party_index[id] = index++;

		for (GW::HeroPartyMember hero : info->heroes) {
			if (hero.ownerplayerid == player.loginnumber) {
				party_index[hero.agentid] = index++;
			}
		}
	}
}

void PartyDamage::Draw() {
	if (!isVisible_) return;

	int size = GW::Partymgr().GetPartySize();
	if (size > MAX_PLAYERS) size = MAX_PLAYERS;
	if (party_size_ != size) {
		party_size_ = size;
		SetSize(Drawing::SizeI(WIDTH, party_size_ * line_height_));
		for (int i = 0; i < MAX_PLAYERS; ++i) {
			bool visible = (i < party_size_);
			absolute[i]->SetVisible(visible);
			percent[i]->SetVisible(visible);
			bar[i]->SetVisible(visible);
			recent[i]->SetVisible(visible);
		}
	}

	// reset recent if needed
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if (TBTimer::diff(damage[i].last_damage) > RECENT_MAX_TIME) {
			damage[i].recent_damage = 0;
		}
	}

	long max_recent = 0;
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if (max_recent < damage[i].recent_damage) {
			max_recent = damage[i].recent_damage;
		}
	}

	long max = 0;
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if (max < damage[i].damage) {
			max = damage[i].damage;
		}
	}

	const int BUF_SIZE = 10;
	wchar_t buff[BUF_SIZE];
	for (int i = 0; i < party_size_; ++i) {
		if (damage[i].damage < 1000) {
			swprintf_s(buff, BUF_SIZE, L"%d", damage[i].damage);
		} else if (damage[i].damage < 1000 * 10) {
			swprintf_s(buff, BUF_SIZE, L"%.2f k", (float)damage[i].damage / 1000);
		} else if (damage[i].damage < 1000 * 1000) {
			swprintf_s(buff, BUF_SIZE, L"%.1f k", (float)damage[i].damage / 1000);
		} else {
			swprintf_s(buff, BUF_SIZE, L"%.1f mil", (float)damage[i].damage / 1000000);
		}
		absolute[i]->SetText(buff);

		float perc_of_total = GetPercentageOfTotal(damage[i].damage);
		swprintf_s(buff, BUF_SIZE, L"%.1f %%", perc_of_total);
		percent[i]->SetText(buff);

		float part_of_max = 0;
		if (max > 0) {
			part_of_max = (float)(damage[i].damage) / max;
		}
		bar[i]->SetSize(SizeI(std::lround(WIDTH * part_of_max), line_height_));
		bar[i]->SetLocation(PointI(std::lround(WIDTH * (1 - part_of_max)), i * line_height_));

		float part_of_recent = 0;
		if (max_recent > 0) {
			part_of_recent = (float)(damage[i].recent_damage) / max_recent;
		}
		recent[i]->SetSize(SizeI(std::lround(WIDTH * part_of_recent), RECENT_HEIGHT));
		recent[i]->SetLocation(PointI(std::lround(WIDTH * (1 - part_of_recent)),
			(i + 1) * line_height_ - RECENT_HEIGHT));

		Drawing::Color inactive = labelcolor - Drawing::Color(0.0f, 0.3f, 0.3f, 0.3f);
		if (damage[i].damage == 0 
			|| GW::Map().GetInstanceType() == GW::Constants::InstanceType::Outpost
			|| GW::Agents().GetAgentByID(damage[i].agent_id) == nullptr) {

			absolute[i]->SetForeColor(inactive);
			percent[i]->SetForeColor(inactive);
		} else {
			absolute[i]->SetForeColor(labelcolor);
			percent[i]->SetForeColor(labelcolor);
		}
	}
}

void PartyDamage::SaveLocation() {
	CalculateAbsoluteLocation();
	int x = absoluteLocation_.X;
	int y = absoluteLocation_.Y;
	Config::IniWriteLong(PartyDamage::IniSection(), PartyDamage::IniKeyX(), x);
	Config::IniWriteLong(PartyDamage::IniSection(), PartyDamage::IniKeyY(), y);
}

float PartyDamage::GetPartOfTotal(long dmg) const {
	if (total == 0) return 0;
	return (float)dmg / total;
}

void PartyDamage::WritePartyDamage() {
	vector<size_t> idx(MAX_PLAYERS);
	for (size_t i = 0; i < MAX_PLAYERS; ++i) idx[i] = i;
	sort(idx.begin(), idx.end(), [this](size_t i1, size_t i2) {
		return damage[i1].damage > damage[i2].damage;
	});

	for (size_t i = 0; i < idx.size(); ++i) {
		WriteDamageOf(idx[i], i + 1);
	}
	send_queue.push(L"Total ~ 100 % ~ " + std::to_wstring(total));
}

void PartyDamage::WriteDamageOf(int index, int rank) {
	if (index >= MAX_PLAYERS) return;
	if (index < 0) return;
	if (damage[index].damage <= 0) return;

	if (rank == 0) {
		rank = 1; // start at 1, add 1 for each player with higher damage
		for (int i = 0; i < MAX_PLAYERS; ++i) {
			if (i == index) continue;
			if (damage[i].agent_id == 0) continue;
			if (damage[i].damage > damage[index].damage) ++rank;
		}
	}

	const int size = 130;
	wchar_t buff[size];
	swprintf_s(buff, size, L"#%2d ~ %3.2f %% ~ %ls/%ls %ls ~ %d",
		rank,
		GetPercentageOfTotal(damage[index].damage),
		GW::Constants::GetWProfessionAcronym(damage[index].primary).c_str(),
		GW::Constants::GetWProfessionAcronym(damage[index].secondary).c_str(),
		damage[index].name.c_str(),
		damage[index].damage);

	send_queue.push(buff);
}


void PartyDamage::WriteOwnDamage() {
	GW::Agent* me = GW::Agents().GetPlayer();
	if (me == nullptr) return;

	auto cause_it = party_index.find(me->Id);
	if (cause_it == party_index.end()) return;

	int index = cause_it->second;
	WriteDamageOf(index);
}

void PartyDamage::ResetDamage() {
	total = 0;
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		damage[i].Reset();
	}
}

void PartyDamage::SetFreeze(bool b) {
	Control::SetEnabled(!b);
	containerPanel_->SetEnabled(!b);
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		absolute[i]->SetEnabled(!b);
		percent[i]->SetEnabled(!b);
	}
}

void PartyDamage::SetTransparentBackColor(bool b) {
	if (b) {
		SetBackColor(Color::Empty());
	} else {
		Drawing::Theme::ControlTheme theme = Application::InstancePtr()
			->GetTheme().GetControlColorTheme(PartyDamage::ThemeKey());
		SetBackColor(theme.BackColor);
	}
}

void PartyDamage::LoadIni() {
	inifile_ = new CSimpleIni(false, false, false);
	inifile_->LoadFile(GuiUtils::getPath(inifilename).c_str());
	CSimpleIni::TNamesDepend keys;
	inifile_->GetAllKeys(inisection, keys);
	for (CSimpleIni::Entry key : keys) {
		const wchar_t* wkey = key.pItem;
		try {
			long lkey = std::stol(wkey);
			if (lkey <= 0) continue;
			long lval = inifile_->GetLongValue(inisection, wkey, 0);
			if (lval <= 0) continue;
			hp_map[lkey] = lval;
		} catch (...) {
			// no big deal
		}
	}
}

void PartyDamage::SaveIni() {
	for (const std::pair<DWORD, long>& item : hp_map) {
		wstring key = std::to_wstring(item.first);
		inifile_->SetLongValue(inisection, key.c_str(), item.second, 0, false, true);
	}
	inifile_->SaveFile(GuiUtils::getPath(inifilename).c_str());
}
