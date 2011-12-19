//
//  GameScene.h
//  LastDino
//
//  Created by Joel Davis on 12/16/11.
//  Copyright (c) 2011 Joel Davis. All rights reserved.
//

#import "cocos2d.h"

#import "GameSoundManager.h"

@class Platform;

enum 
{
    GameState_PLAYING,
    GameState_DEAD
};

enum 
{
    Land_TREES,
    Land_ROCK,
    Land_LONG
};

@interface GameLayer : CCLayer
{
    float playerYVel;
    
    NSMutableArray *_platforms;
    Platform *_dragPlatform;
    CGPoint _dragStart;
    CGPoint _platformGrabStart;
    int _gameState;
    
    // gameplay
    float _scrollSpeed;    
    float _catchupSpeed;    
    int _coins;
    
    // game dist
    float _gameDist;
    
    NSMutableArray *_levelCoins;
    
    NSMutableArray *_coinAnimFrames;
    CCAnimation *_coinAnim;
    
    // level build
    
    // audio
    SimpleAudioEngine *_soundEngine;
}

@property (nonatomic, retain) CCSprite *bgImage;
@property (nonatomic, retain) CCSprite *midgroundImage;
@property (nonatomic, retain) CCSprite *player;
@property (nonatomic, retain) CCSprite *target;

@property (nonatomic, retain) CCLabelBMFont *coinsLabel;
@property (nonatomic, retain) CCLabelBMFont *distLabel;

@property (nonatomic, retain) CCAction *animRun;

@property (nonatomic, retain) CCAction *animCoins;

// returns a CCScene that contains the HelloWorldLayer as the only child
+(CCScene *) scene;

// gameplay stuff
- (void) getCoin: (CCSprite*)coin;

// build platforms
- (void) buildPlatforms;

@end
