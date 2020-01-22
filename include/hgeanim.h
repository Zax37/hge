/*
** Haaf's Game Engine 1.7
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** hgeAnimation helper class header
*/


#ifndef HGEANIM_H
#define HGEANIM_H


#include "hgesprite.h"


#define HGEANIM_FWD			0
#define HGEANIM_REV			1
#define HGEANIM_PINGPONG	2
#define HGEANIM_NOPINGPONG	0
#define HGEANIM_LOOP		4
#define HGEANIM_NOLOOP		0


/*
** HGE Animation class
*/
class hgeAnimation : public hgeSprite
{
public:
	hgeAnimation(HTEXTURE tex, int nframes, float FPS, float x, float y, float w, float h);
	hgeAnimation(const hgeAnimation &anim);
	
	void		Play();
	void		Stop() { is_playing_=false; }
	void		Resume() { is_playing_=true; }
	void		Update(float fDeltaTime);
	bool		IsPlaying() const { return is_playing_; }

	void		SetTexture(HTEXTURE tex) { hgeSprite::SetTexture(tex); orig_width_ = hge_->Texture_GetWidth(tex, true); }
	void		SetTextureRect(float x1, float y1, float x2, float y2) { hgeSprite::SetTextureRect(x1,y1,x2,y2); SetFrame(cur_frame_); }
	void		SetMode(int mode);
	void		SetSpeed(float FPS) { speed_= 1.0f / FPS; }
	void		SetFrame(int n);
	void		SetFrames(int n) { frames_=n; }

	int			GetMode() const { return mode_; }
	float		GetSpeed() const { return 1.0f / speed_; }
	int			GetFrame() const { return cur_frame_; }
	int			GetFrames() const { return frames_; }

private:
	hgeAnimation();

	int			orig_width_;

	bool		is_playing_;

	float		speed_;
	float		since_last_frame_;

	int			mode_;
	int			delta_;
	int			frames_;
	int			cur_frame_;
};


#endif
