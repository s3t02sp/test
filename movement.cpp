#include "movement.hpp"

constexpr int kMovementKeyBits =
	IN_TURNLEFT |
	IN_FORWARD |
	IN_BACK |
	IN_TURNRIGHT |
	IN_MOVELEFT |
	IN_MOVERIGHT |
	IN_JUMP;

void c_movement::fix_cmd_buttons(const bool calling_from_strafer) {
	if (!g_ctx->m_local_pawn)
		return;

	if (!(g_ctx->m_local_pawn->m_flags() & FL_ONGROUND) && !calling_from_strafer)
		return;

	if (g_ctx->m_user_cmd->pb.mutable_base()->forwardmove() > 0)
		g_ctx->m_user_cmd->m_button_state.m_button_state |= IN_FORWARD;
	else if (g_ctx->m_user_cmd->pb.mutable_base()->forwardmove() < 0)
		g_ctx->m_user_cmd->m_button_state.m_button_state |= IN_BACK;

	if (g_ctx->m_user_cmd->pb.mutable_base()->leftmove() > 0)
		g_ctx->m_user_cmd->m_button_state.m_button_state |= IN_MOVELEFT;
	else if (g_ctx->m_user_cmd->pb.mutable_base()->leftmove() < 0)
		g_ctx->m_user_cmd->m_button_state.m_button_state |= IN_MOVERIGHT;
}

float c_movement::apply_friction(vec3_t& vVecVelocity)
{
	auto pawn = g_ctx->m_local_pawn;
	if (!pawn)
		return 0.0f;

	auto pMoveService = pawn->m_movement_services();
	if (!pMoveService)
		return 0.0f;

	static float sv_friction = g_interfaces->m_var->get_by_name("sv_friction")->get_float();
	static float sv_stopspeed = g_interfaces->m_var->get_by_name("sv_stopspeed")->get_float();

	float flSpeed = 0.0f;

	if (g_interfaces->m_global_vars->m_current_time > pMoveService->m_offset_tick_complete_time())
		flSpeed = vVecVelocity.length();
	else
		flSpeed = pMoveService->m_offset_tick_stashed_speed();

	float flSpeedReduction = 0.0f;

	if (flSpeed >= 0.1f) {
		if (pawn->m_ground_entity().get()) {
			float friction = pMoveService->m_surface_friction() * sv_friction;
			flSpeedReduction = std::fmax(flSpeed, sv_stopspeed) * friction * pawn->m_friction() * g_interfaces->m_global_vars->m_interval_per_tick;
		}
		float flNewSpeed = fmaxf(flSpeed - flSpeedReduction, 0.f);
		if (flNewSpeed != flSpeed)
		{
			vVecVelocity *= flNewSpeed / flSpeed;
		}
	}
	return fmaxf(-(flSpeed - flSpeedReduction), 0.0);
}

vec3_t c_movement::simulate_pos_at_stop(vec3_t vPos)
{
	vec3_t vVecVelocity = g_ctx->m_local_pawn->m_vec_velocity();
	vVecVelocity.z = 0.f;

	if (!g_cfg->rage_bot.m_auto_stop_early)
		return vPos;

	if (g_interfaces->m_var->get_by_name("weapon_accuracy_nospread")->get_int())
		return vPos;

	if (!(g_ctx->m_local_pawn->m_flags() & FL_ONGROUND))
		return vPos;

	static float sv_accelerate = g_interfaces->m_var->get_by_name("sv_accelerate")->get_float();
	static float sv_accelerate_use_weapon_speed = g_interfaces->m_var->get_by_name("sv_accelerate_use_weapon_speed")->get_float();
	static float sv_water_slow_amount = g_interfaces->m_var->get_by_name("sv_water_slow_amount")->get_float();

	float flSurfaceFriction = g_ctx->m_local_pawn->m_movement_services()->m_surface_friction();

	int nMaxSimTicks = 20;

	auto pPlayer = g_ctx->m_local_pawn;
	auto pMoveService = pPlayer->m_movement_services();
	auto pWeapon = g_ctx->m_weapon;

	vec3_t vExtrap = vPos;

	for (int i = 0; i < nMaxSimTicks; i++) {

		if (vVecVelocity.length_2d() <= pWeapon->get_max_speed() * 0.34f)
			break;

		float flFrictionDecel = apply_friction(vVecVelocity);

		vec3_t vWishDir = -vVecVelocity;
		float flWishSpeed = vWishDir.normalize_in_place();
		float flCurrentSpeed = vVecVelocity.dot(vWishDir);

		int nButtonFlags = g_ctx->m_user_cmd->m_button_state.m_button_state;
		bool bIsDucking = (nButtonFlags & IN_DUCK) ||
			pMoveService->m_ducking() ||
			(pPlayer->m_flags() & FL_DUCKING);
		bool bIsHoldingSprintKey = (nButtonFlags & IN_SPEED) && !bIsDucking;

		float v19_clampedWishSpeed = std::max(250.0f, flWishSpeed);
		float v20_effectiveAbsMaxSpeed = v19_clampedWishSpeed;
		float v21_accelerationFactor = 1.0f;
		bool v23_weaponCausesSlowdown = false;

		if (sv_accelerate_use_weapon_speed && pWeapon) {
			float flWeaponMaxSpeed = pWeapon->get_max_speed();
			auto pVData = pWeapon->get_weapon_base_vdata();

			if (pVData && pWeapon->m_zoom_level() > 0 && pVData->m_zoom_levels() > 1 && (flWeaponMaxSpeed * 0.51999998f) < 110.0f) {
				v23_weaponCausesSlowdown = true;
			}

			float flWeaponSpeedRatio = std::min(1.0f, flWeaponMaxSpeed / 250.0f);

			if ((!bIsDucking && !bIsHoldingSprintKey) || v23_weaponCausesSlowdown) {
				v21_accelerationFactor = flWeaponSpeedRatio;
			}

			v20_effectiveAbsMaxSpeed = v19_clampedWishSpeed * flWeaponSpeedRatio;
		}

		float v29_baseSpeedForAccelCalc = v19_clampedWishSpeed;

		unsigned int uWaterLevel = (pPlayer->m_water_level() * 4.f) + 1.f;

		if (uWaterLevel >= 2) {
			float flWaterSlowAmount = sv_water_slow_amount;
			v20_effectiveAbsMaxSpeed *= flWaterSlowAmount;

			if (bIsDucking) {
				v20_effectiveAbsMaxSpeed *= 0.34f;
				v21_accelerationFactor = std::min(0.34f, v21_accelerationFactor);
			}
			else {
				if (!bIsHoldingSprintKey) {
					v29_baseSpeedForAccelCalc = v19_clampedWishSpeed * flWaterSlowAmount;
				}
			}
		}
		else {
			if (bIsDucking) {
				v20_effectiveAbsMaxSpeed *= 0.34f;
				v21_accelerationFactor = std::min(0.34f, v21_accelerationFactor);
			}
		}

		float v29_finalSpeedCap = v29_baseSpeedForAccelCalc * v21_accelerationFactor;

		float flModifiedSvAccelerate = sv_accelerate;
		float flPositiveCurrentSpeed = std::max(0.0f, flCurrentSpeed);

		if (bIsHoldingSprintKey) {
			bool bIsCarryingHostage = pPlayer->m_hostage_services() ? pPlayer->m_hostage_services()->m_carried_hostage().get() : false;

			if (!bIsCarryingHostage && !v23_weaponCausesSlowdown) {
				v29_finalSpeedCap *= 0.51999998f;
			}

			float flReducedAbsMaxSpeed = v20_effectiveAbsMaxSpeed * 0.51999998f;
			float flSprintSpeedThreshold = flReducedAbsMaxSpeed - 5.0f;

			if (flPositiveCurrentSpeed > flSprintSpeedThreshold) {
				float flNumerator = std::max(0.0f, flPositiveCurrentSpeed - flSprintSpeedThreshold);
				float flDenominator = std::max(0.001f, flReducedAbsMaxSpeed - flSprintSpeedThreshold);
				float flSpeedFactorRatio = flNumerator / flDenominator;
				float flAccelReductionFactor = std::min(1.0f, std::max(0.0f, 1.0f - flSpeedFactorRatio));
				flModifiedSvAccelerate *= flAccelReductionFactor;
			}
		}

		float flPotentialAccelGain = flModifiedSvAccelerate * INTERVAL_PER_TICK * v29_finalSpeedCap * flSurfaceFriction;
		float flAddSpeed = -flCurrentSpeed;
		float flAccelAmountToAdd = std::max(0.0f, std::min(flPotentialAccelGain, flAddSpeed));

		vVecVelocity.x += vWishDir.x * flAccelAmountToAdd;
		vVecVelocity.y += vWishDir.y * flAccelAmountToAdd;

		vExtrap.x += vVecVelocity.x * INTERVAL_PER_TICK;
		vExtrap.y += vVecVelocity.y * INTERVAL_PER_TICK;
	}

	return vExtrap;
}

void c_movement::stop_movement()
{
	static bool weapon_accuracy_nospread = g_interfaces->m_var->get_by_name("weapon_accuracy_nospread")->get_bool();

	bool bQuickStop = (g_cfg->misc.m_quick_stop && !g_ctx->m_is_pressing_movement_key);
	if (!bQuickStop && weapon_accuracy_nospread)
		return;

	if (bQuickStop || this->m_wants_2d_move_halt)
	{
		bool bOnGround = g_ctx->m_local_pawn->m_flags() & FL_ONGROUND;

		if (!bOnGround && !g_cfg->rage_bot.m_auto_stop_in_air)
			return;

		auto pBaseCmd = g_ctx->m_user_cmd->pb.mutable_base();

		vec3_t vVelocity = g_ctx->m_local_pawn->m_vec_velocity();

		pBaseCmd->set_leftmove(0.0f);
		pBaseCmd->set_forwardmove(vVelocity.length_2d() > 20.0f ? 1.0f : 0.0f);

		vec3_t aViewAngles = this->m_ang_camera_angles;
		float flYaw = g_math->calc_angle(vec3_t(0.0f, 0.0f, 0.0f), vVelocity).y + 180.0f;
		float flRotation = DirectX::XMConvertToRadians(aViewAngles.y - flYaw);

		float flCosRotation = cos(flRotation);
		float flSinRotation = sin(flRotation);

		float flNewForwardMove = flCosRotation * pBaseCmd->forwardmove() - flSinRotation * pBaseCmd->leftmove();
		float flNewSideMove = flSinRotation * pBaseCmd->forwardmove() + flCosRotation * pBaseCmd->leftmove();

		pBaseCmd->set_forwardmove(flNewForwardMove);
		pBaseCmd->set_leftmove(-flNewSideMove);

		g_ctx->m_user_cmd->m_button_state.m_button_state &= ~kMovementKeyBits;
		g_ctx->m_user_cmd->m_button_state.m_button_state2 &= ~kMovementKeyBits;
		g_ctx->m_user_cmd->m_button_state.m_button_state3 &= ~kMovementKeyBits;
	}
}

bool c_movement::is_valid_movement_trace(GameTrace_t& tr, BBox_t bounds, TraceFilter_t* filter)
{
	GameTrace_t stuck;

	if (tr.m_all_solid)
	{
		return false;
	}

	// We hit something but no valid plane data?
	if (tr.m_fraction < 1.0f && fabs(tr.m_normal.x) < FLT_EPSILON && fabs(tr.m_normal.y) < FLT_EPSILON
		&& fabs(tr.m_normal.z) < FLT_EPSILON)
	{
		return false;
	}

	// Is the plane deformed?
	if (fabs(tr.m_normal.x) > 1.0f || fabs(tr.m_normal.y) > 1.0f || fabs(tr.m_normal.z) > 1.0f)
	{
		return false;
	}

	// Do an unswept trace and a backward trace just to be sure.
	g_interfaces->m_vphys2_world->trace_player_bbox(&tr.m_end_pos, &tr.m_end_pos, &bounds, filter, &stuck);
	if (stuck.m_all_solid || stuck.m_fraction < 1.0f - FLT_EPSILON)
	{
		return false;
	}

	g_interfaces->m_vphys2_world->trace_player_bbox(&tr.m_end_pos, &tr.m_start_pos, &bounds, filter, &stuck);
	if (stuck.m_all_solid)
	{
		return false;
	}

	return true;
}

void c_movement::bunnyhop(c_user_cmd* user_cmd)
{
	if (!g_cfg->misc.m_bunny_hop)
		return;

	static bool sv_autobunnyhopping = g_interfaces->m_var->get_by_name("sv_autobunnyhopping")->get_bool();

	if (sv_autobunnyhopping || !(user_cmd->m_button_state.m_button_state & IN_JUMP))
		return;

	auto pMovementServices = g_ctx->m_local_pawn->m_movement_services();
	if (!pMovementServices)
		return;

	if ((g_ctx->m_local_pawn->m_flags() & FL_ONGROUND))
	{
		user_cmd->m_button_state.m_button_state &= ~IN_JUMP;
	}
}

void c_movement::jump_bug(c_user_cmd* user_cmd)
{
	if (!g_cfg->misc.m_jump_bug)
		return;

	if (!(user_cmd->m_button_state.m_button_state & IN_JUMP))
		return;

	auto pawn = g_ctx->m_local_pawn;
	if (!pawn)
		return;

	auto movement = pawn->m_movement_services();
	if (!movement)
		return;

	bool edge_detected = false;
	bool pre_on_ground = pawn->m_flags() & FL_ONGROUND;
	bool post_on_ground = g_ctx->m_predicted_flags & FL_ONGROUND;

	if (pre_on_ground && !post_on_ground)
	{
		vec3_t origin = pawn->m_game_scene_node()->m_abs_origin();

		auto collision = pawn->m_collision();
		if (collision)
		{
			vec3_t mins = collision->m_mins();
			vec3_t maxs = collision->m_maxs();

			TraceFilter_t filter;
			g_interfaces->m_vphys2_world->init_player_movement_trace_filter(&filter, pawn, 0x1C3003, COLLISION_GROUP_PLAYER_MOVEMENT);

			vec3_t start = origin;
			vec3_t end = origin - vec3_t(0, 0, 32.0f);

			GameTrace_t trace;
			BBox_t bounds = { mins, maxs };

			if (g_interfaces->m_vphys2_world->trace_player_bbox(&start, &end, &bounds, &filter, &trace))
			{
				edge_detected = trace.m_fraction >= 1.0f || !trace.m_hit_entity;
			}
		}
	}

	if ((pre_on_ground && !post_on_ground) || edge_detected)
	{
		user_cmd->m_button_state.m_button_state |= IN_DUCK;

		auto base_cmd = g_ctx->m_user_cmd->pb.mutable_base();
		base_cmd->clear_subtick_moves();

		if (auto duck_subtick = g_protobuf->add_subtick_move_step(user_cmd))
		{
			duck_subtick->set_when(0.f);
			duck_subtick->set_button(IN_DUCK);
			duck_subtick->set_pressed(true);
		}

		if (auto jump_subtick = g_protobuf->add_subtick_move_step(user_cmd))
		{
			jump_subtick->set_when(0.25f);
			jump_subtick->set_button(IN_JUMP);
			jump_subtick->set_pressed(true);
		}

		if (auto unduck_subtick = g_protobuf->add_subtick_move_step(user_cmd))
		{
			unduck_subtick->set_when(0.999f);
			unduck_subtick->set_button(IN_DUCK);
			unduck_subtick->set_pressed(false);
		}

		if (auto unjump_subtick = g_protobuf->add_subtick_move_step(user_cmd))
		{
			unjump_subtick->set_when(0.999f);
			unjump_subtick->set_button(IN_JUMP);
			unjump_subtick->set_pressed(false);
		}
	}
	else if (!(pawn->m_flags() & FL_ONGROUND))
	{
		user_cmd->m_button_state.m_button_state &= ~IN_DUCK;
	}
}

void c_movement::directional_air_strafe(vec3_t vVelocity, float flCurrentSpeed, float flFrameTime, bool bSubtickCalling)
{
	if (g_cfg->misc.m_auto_strafe_exploit && !bSubtickCalling)
		return;

	if (!g_cfg->misc.m_auto_strafe && !bSubtickCalling)
		return;

	static float sv_airaccelerate = g_interfaces->m_var->get_by_name("sv_airaccelerate")->get_float();
	static float sv_air_max_wishspeed = g_interfaces->m_var->get_by_name("sv_air_max_wishspeed")->get_float();

	auto pBase = g_ctx->m_user_cmd->pb.mutable_base();
	auto pMoveService = g_ctx->m_local_pawn->m_movement_services();

	static uint64_t uLastPressed = 0;
	static uint64_t uLastButtons = 0;

	const uint64_t uCurrentButtons = g_ctx->m_user_cmd->m_button_state.m_button_state;

	const auto CheckButton = [&](const uint64_t uButton)
		{
			if (uCurrentButtons & uButton && (!(uLastButtons & uButton)
				|| (uButton & IN_MOVELEFT && !(uLastPressed & IN_MOVERIGHT))
				|| (uButton & IN_MOVERIGHT && !(uLastPressed & IN_MOVELEFT))
				|| (uButton & IN_FORWARD && !(uLastPressed & IN_BACK))
				|| (uButton & IN_BACK && !(uLastPressed & IN_FORWARD))))
			{
				if (uButton & IN_MOVELEFT)
					uLastPressed &= ~IN_MOVERIGHT;
				else if (uButton & IN_MOVERIGHT)
					uLastPressed &= ~IN_MOVELEFT;
				else if (uButton & IN_FORWARD)
					uLastPressed &= ~IN_BACK;
				else if (uButton & IN_BACK)
					uLastPressed &= ~IN_FORWARD;

				uLastPressed |= uButton;
			}
			else if (!(uCurrentButtons & uButton))
				uLastPressed &= ~uButton;
		};

	CheckButton(IN_MOVELEFT);
	CheckButton(IN_MOVERIGHT);
	CheckButton(IN_FORWARD);
	CheckButton(IN_BACK);

	uLastButtons = uCurrentButtons;

	if (uCurrentButtons & IN_SPEED || g_ctx->m_local_pawn->m_actual_move_type() == movetype_ladder ||
		g_ctx->m_local_pawn->m_actual_move_type() == movetype_noclip ||
		g_ctx->m_local_pawn->m_flags() & FL_ONGROUND)
		return;

	float flOffset = 0.f;
	if (uLastPressed & IN_MOVELEFT)
		flOffset += 90.f;

	if (uLastPressed & IN_MOVERIGHT)
		flOffset -= 90.f;

	if (uLastPressed & IN_FORWARD)
		flOffset *= 0.5f;

	else if (uLastPressed & IN_BACK)
		flOffset = -flOffset * 0.5f + 180.f;

	float flYaw = g_math->normalize_yaw(g_interfaces->m_csgo_input->get_view_angles().y);

	flYaw += flOffset;

	pBase->set_forwardmove(0.f);
	pBase->set_leftmove(0.f);

	const float flVelocityAngle = g_math->normalize_yaw(DirectX::XMConvertToDegrees(atan2f(vVelocity.y, vVelocity.x)));
	const float flSpeed = vVelocity.length_2d();
	const auto flIdeal = std::clamp(
		DirectX::XMConvertToDegrees(
			atan(
				fmax(
					15.f,
					30.f - (
						flCurrentSpeed *
						sv_airaccelerate *
						pMoveService->m_surface_friction() *
						flFrameTime
						)
				) / flSpeed
			)
		),
		0.f,
		45.f
	);

	const float flVelocityDelta = g_math->normalize_yaw(flYaw - flVelocityAngle);

	auto RotateMovement = [](c_user_cmd* pCmd, CBaseUserCmdPB* pBaseCmd, float flTargetYaw)
		{
			const float flRotation = DirectX::XMConvertToRadians(pBaseCmd->viewangles().y() - flTargetYaw);

			const float flNewForwardMove = cos(flRotation) * pBaseCmd->forwardmove() - sin(flRotation) * pBaseCmd->leftmove();
			const float flNewSideMove = sin(flRotation) * pBaseCmd->forwardmove() + cos(flRotation) * pBaseCmd->leftmove();

			pBaseCmd->set_forwardmove(std::clamp(flNewForwardMove, -1.f, 1.f));
			pBaseCmd->set_leftmove(std::clamp(flNewSideMove * -1.f, -1.f, 1.f));
		};

	if ((fabsf(flVelocityDelta) > 170.f && flSpeed > 80.f) || (flVelocityDelta > flIdeal && flSpeed > 80.f))
	{
		flYaw = flIdeal + flVelocityAngle;
		pBase->set_leftmove(-1.f);
		RotateMovement(g_ctx->m_user_cmd, pBase, g_math->normalize_yaw(flYaw));

		return;
	}

	static bool bSideSwitch = false;
	bSideSwitch = bSubtickCalling ? !bSideSwitch : g_ctx->m_user_cmd->m_command_number % 2 == 0;

	if (-flIdeal <= flVelocityDelta || flSpeed <= 80.f) {
		if (bSideSwitch) {
			flYaw = flYaw - flIdeal;
			pBase->set_leftmove(-1.f);
		}
		else {
			flYaw = flIdeal + flYaw;
			pBase->set_leftmove(1.f);
		}
	}
	else {
		flYaw = flVelocityAngle - flIdeal;
		pBase->set_leftmove(1.f);
	}

	RotateMovement(g_ctx->m_user_cmd, pBase, flYaw);
}

void c_movement::air_accelerate(vec3_t& vVelocity, vec3_t vMoveImpulse, float& flStamina, float flFrameTime, float flYaw)
{
	static float sv_airaccelerate = g_interfaces->m_var->get_by_name("sv_airaccelerate")->get_float();
	static float sv_staminarecoveryrate = g_interfaces->m_var->get_by_name("sv_staminarecoveryrate")->get_float();
	static float sv_air_max_wishspeed = g_interfaces->m_var->get_by_name("sv_air_max_wishspeed")->get_float();

	float flCurrentSpeed = vVelocity.length_2d();
	if (flStamina > 0)
	{
		float flSpeedScale = std::clamp(1.0f - (flStamina / 100.f), 0.f, 1.f);
		flCurrentSpeed *= flSpeedScale * flSpeedScale;
		flStamina = fmaxf(flStamina - (flFrameTime * sv_staminarecoveryrate), 0.f);
	}

	vec3_t angViewAngles = { 0, flYaw, 0 };

	vec3_t vecForward, vecLeft, vecUp;
	g_math->angle_vectors(angViewAngles, vecForward, vecLeft, vecUp);
	vecForward.normalize_in_place();
	vecLeft.normalize_in_place();

	vec3_t vecWishDir(
		((vecForward.x * vMoveImpulse.x) * flCurrentSpeed) - ((vecLeft.x * vMoveImpulse.y) * flCurrentSpeed),
		((vecForward.y * vMoveImpulse.x) * flCurrentSpeed) - ((vecLeft.y * vMoveImpulse.y) * flCurrentSpeed),
		0.f
	);

	float flWishSpeed = vecWishDir.normalize_in_place();
	flWishSpeed = fminf(flWishSpeed, flCurrentSpeed);

	const float flSpeedAdd = fminf(flWishSpeed, sv_air_max_wishspeed - vVelocity.dot(vecWishDir));

	if (flSpeedAdd < 0.f)
		return;

	const float flAccelSpeed = fminf(flSpeedAdd, flWishSpeed * flFrameTime * sv_airaccelerate);
	vVelocity += (vecWishDir * flAccelSpeed) * 0.5f;
}

void c_movement::auto_strafe(vec3_t vVelocity, float flCurrentSpeed, float flFrameTime, bool bSubtickCalling)
{
	directional_air_strafe(vVelocity, flCurrentSpeed, flFrameTime, bSubtickCalling);
}

void c_movement::parachute_accelerate(vec3_t& out_velo, float& stamina, float& friction, vec3_t move, float frametime)
{
	static float sv_air_max_wishspeed = g_interfaces->m_var->get_by_name("sv_air_max_wishspeed")->get_float();
	static float sv_air_accelerate = g_interfaces->m_var->get_by_name("sv_airaccelerate")->get_float();
	static float sv_staminarecoveryrate = g_interfaces->m_var->get_by_name("sv_staminarecoveryrate")->get_float();

	float max_speed = out_velo.length_2d();
	if (stamina > 0)
	{
		float flSpeedScale = std::clamp(1.0f - (stamina / 100.f), 0.f, 1.f);
		max_speed *= flSpeedScale * flSpeedScale;
		stamina = fmaxf(stamina - (frametime * sv_staminarecoveryrate), 0.f);
	}

	vec3_t forward, left, up;
	auto wish_angles = this->m_ang_model_angles;
	g_math->angle_vectors(wish_angles, forward, left, up);
	forward.normalize_in_place();
	left.normalize_in_place();

	vec3_t wish_dir{};
	wish_dir.x = (((forward.x * move.x) * max_speed) - ((left.x * move.y) * max_speed));
	wish_dir.y = ((forward.y * move.x) * max_speed) - ((left.y * move.y) * max_speed);

	auto wish_speed = wish_dir.normalize_in_place();

	wish_speed = fminf(wish_speed, max_speed);

	const float spped_add = fminf(wish_speed, sv_air_max_wishspeed - out_velo.dot(wish_dir));

	if (spped_add > 0.f)
	{
		const float accelspd = fminf(spped_add, ((wish_speed * frametime) * sv_air_accelerate) * friction);
		out_velo += wish_dir * accelspd;
	}

	friction = 1.0f;
	if (g_ctx->m_local_pawn->m_flags() & FL_ONGROUND)
	{
		if (out_velo.z <= 140.0 && out_velo.z > 0.0)
			friction = 0.25f;
	}
}

void c_movement::subtick_strafer()
{
	if (!g_cfg->misc.m_auto_strafe_exploit)
		return;

	if (g_ctx->m_user_cmd->m_button_state.m_button_state & IN_SPEED || g_ctx->m_local_pawn->m_flags() & FL_ONGROUND)
		return;

	if (g_ctx->m_local_pawn->m_actual_move_type() == movetype_ladder ||
		g_ctx->m_local_pawn->m_actual_move_type() == movetype_noclip)
		return;

	if (!g_ctx->m_local_pawn || !g_ctx->m_local_pawn->is_alive() || !g_ctx->m_local_pawn->m_movement_services())
		return;

	auto movement_servics = g_ctx->m_local_pawn->m_movement_services();
	if (!movement_servics)
		return;

	auto pBase = g_ctx->m_user_cmd->pb.mutable_base();
	auto pLocal = g_ctx->m_local_pawn;
	auto pMoveService = pLocal->m_movement_services();

	pBase->clear_subtick_moves();

	float flFriction = pMoveService->m_surface_friction();
	float flStamina = pMoveService->m_flStamina();

	vec3_t vAbsVelocity = pLocal->m_vec_abs_velocity();
	vec3_t vLastImpulses = pMoveService->m_last_movement_impulses();
	vec3_t vCMDMoveBackup = { pBase->forwardmove(), pBase->leftmove(), pBase->upmove() };

	const int nTicks = std::clamp(32 - pBase->subtick_moves_size(), 0, 32);
	const float flFrameTime = INTERVAL_PER_TICK / static_cast<float>(nTicks);
	float flMovementYaw = this->m_ang_model_angles.y;

	for (int i = 0; i < nTicks; ++i)
	{
		auto pSubtick = g_protobuf->add_subtick_move_step(g_ctx->m_user_cmd);
		if (!pSubtick)
			continue;

		pBase->set_forwardmove(vCMDMoveBackup.x);
		pBase->set_leftmove(vCMDMoveBackup.y);
		pBase->set_upmove(vCMDMoveBackup.z);

		this->air_accelerate(vAbsVelocity, vLastImpulses, flStamina, flFrameTime, flMovementYaw);

		if (this->m_wants_2d_move_halt) {
			this->stop_movement();
			this->movement_fix();
		}
		else {
			this->directional_air_strafe(vAbsVelocity, vAbsVelocity.length_2d(), flFrameTime, true);
		}

		pSubtick->set_when(static_cast<float>(i) / static_cast<float>(nTicks));
		pSubtick->set_analog_forward_delta(pBase->forwardmove() - vLastImpulses.x);
		pSubtick->set_analog_left_delta(pBase->leftmove() - vLastImpulses.y);
		pSubtick->set_button(0);
		pSubtick->set_pressed(false);

		vLastImpulses.x += pSubtick->analog_forward_delta();
		vLastImpulses.y += pSubtick->analog_left_delta();
	}
}

void c_movement::limit_speed(c_user_cmd* user_cmd, c_cs_player_pawn* local_player, c_base_player_weapon* active_weapon, float max_speed)
{
	c_player_movement_service* movement_services = local_player->m_movement_services();

	if (!movement_services)
		return;

	vec3_t velocity = g_ctx->m_local_pawn->m_vec_abs_velocity();

	float cmd_speed = std::sqrt(
		(user_cmd->pb.mutable_base()->leftmove() * user_cmd->pb.mutable_base()->leftmove())
		+ (user_cmd->pb.mutable_base()->forwardmove() * user_cmd->pb.mutable_base()->forwardmove())
	);

	float speed_2d = velocity.length_2d();

	if (cmd_speed <= 50.f
		&& speed_2d <= 50.f)
		return;

	float accelerate = g_interfaces->m_var->get_by_name("sv_accelerate")->get_float();

	vec3_t view_angles = {
		g_ctx->m_user_cmd->pb.mutable_base()->viewangles().x(),
		g_ctx->m_user_cmd->pb.mutable_base()->viewangles().y(),
		g_ctx->m_user_cmd->pb.mutable_base()->viewangles().z()
	};

	vec3_t forward{}, right{}, up{};
	g_math->angle_vectors(view_angles, forward, right, up);

	float diff = speed_2d - max_speed;
	float wish_speed = max_speed;

	vec3_t direction = { forward.x * user_cmd->pb.mutable_base()->forwardmove() + right.x * user_cmd->pb.mutable_base()->leftmove(),
		forward.y * user_cmd->pb.mutable_base()->forwardmove() + right.y * user_cmd->pb.mutable_base()->leftmove(), 0.f };

	const float max_accelerate = accelerate * INTERVAL_PER_TICK * max(250.f, movement_services->m_max_speed() * movement_services->m_surface_friction());

	if (diff - max_accelerate <= 0.f
		|| speed_2d - max_accelerate - 3.f <= 0.f)
		wish_speed = max_speed;
	else {
		direction = velocity;
		wish_speed = -1.f;
	}

	if (user_cmd->pb.mutable_base()->forwardmove() > 0)
		user_cmd->pb.mutable_base()->set_forwardmove(wish_speed);
	else if (user_cmd->pb.mutable_base()->forwardmove() < 0)
		user_cmd->pb.mutable_base()->set_forwardmove(-wish_speed);

	if (user_cmd->pb.mutable_base()->leftmove() > 0)
		user_cmd->pb.mutable_base()->set_leftmove(wish_speed);
	else if (user_cmd->pb.mutable_base()->leftmove() < 0)
		user_cmd->pb.mutable_base()->set_leftmove(-wish_speed);
}

void c_movement::auto_stop(c_user_cmd* user_cmd, c_cs_player_pawn* local_player, c_base_player_weapon* active_weapon, bool no_spread)
{
	if (!g_cfg->rage_bot.m_auto_stop)
		return;

	if (no_spread)
		return;

	if (!(local_player->m_flags() & FL_ONGROUND))
		return;

	auto remove_button = [&](int button) {
		user_cmd->m_button_state.m_button_state &= ~button;
		user_cmd->m_button_state.m_button_state2 &= ~button;
		user_cmd->m_button_state.m_button_state3 &= ~button;
		};

	remove_button(IN_SPEED);

	float wish_speed = active_weapon->get_max_speed() * 0.25f;

	limit_speed(user_cmd, local_player, active_weapon, wish_speed);
}

void c_movement::movement_fix()
{
	auto pBaseCmd = g_ctx->m_user_cmd->pb.mutable_base();

	int sign = this->m_ang_model_angles.x > 89.f ? -1.f : 1.f;
	this->m_ang_model_angles.normalize();

	vec3_t forward, right, up, old_forward, old_right, old_up;

	g_math->angle_vectors(this->m_ang_model_angles, forward, right, up);

	forward.z = right.z = up.x = up.y = 0.f;

	forward.normalize_in_place();
	right.normalize_in_place();
	up.normalize_in_place();

	g_math->angle_vectors(this->m_ang_camera_angles, old_forward, old_right, old_up);

	old_forward.z = old_right.z = old_up.x = old_up.y = 0.f;

	old_forward.normalize_in_place();
	old_right.normalize_in_place();
	old_up.normalize_in_place();

	forward *= pBaseCmd->forwardmove();
	right *= pBaseCmd->leftmove();
	up *= pBaseCmd->upmove();

	float fixed_forward_move = old_forward.dot(right) + old_forward.dot(forward) + old_forward.dot(up, true);
	float fixed_side_move = old_right.dot(right) + old_right.dot(forward) + old_right.dot(up, true);
	float fixed_up_move = old_up.dot(right, true) + old_up.dot(forward, true) + old_up.dot(up);

	pBaseCmd->set_forwardmove(fixed_forward_move);
	pBaseCmd->set_leftmove(fixed_side_move);
	pBaseCmd->set_upmove(fixed_up_move);

	fixed_forward_move = sign * (old_forward.dot(right) + old_forward.dot(forward));
	fixed_side_move = old_right.dot(right) + old_right.dot(forward);

	pBaseCmd->set_forwardmove(std::clamp(fixed_forward_move, -1.f, 1.f));
	pBaseCmd->set_leftmove(std::clamp(fixed_side_move, -1.f, 1.f));

	fix_cmd_buttons(true);
}

void c_movement::handle_move_stop(bool bHaltMovement)
{
	this->m_wants_2d_move_halt = bHaltMovement;

	if (g_ctx->m_local_pawn->m_flags() & FL_ONGROUND)
	{
		this->stop_movement();
	}
}

void c_movement::handle_directional_strafe()
{
	if (g_cfg->misc.m_auto_strafe_exploit)
		this->subtick_strafer();
	else
		this->directional_air_strafe(g_ctx->m_local_pawn->m_vec_abs_velocity(), g_ctx->m_weapon->get_max_speed(), 1.f, false);
}

void c_movement::on_create_move(c_user_cmd* user_cmd)
{
	if (g_ctx->m_local_pawn->m_move_type() == movetype_ladder || g_ctx->m_local_pawn->m_move_type() == movetype_noclip)
		return;

	bunnyhop(user_cmd);
	jump_bug(user_cmd);
	
	if (g_cfg->misc.m_auto_strafe_exploit)
		subtick_strafer();
	else if (g_cfg->misc.m_auto_strafe)
		auto_strafe(g_ctx->m_local_pawn->m_vec_abs_velocity(), g_ctx->m_weapon->get_max_speed(), INTERVAL_PER_TICK, false);
}
