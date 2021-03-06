import luxe.Mesh;
import luxe.Color;
import luxe.Vector;
import luxe.Quaternion;
import luxe.Input;
import luxe.Camera;
import luxe.Entity;
import luxe.utils.Maths;
import luxe.tween.Actuate;

import Gameboard;

class Creep extends Entity
{
	public var mesh : Mesh;	
	public var creepName : String;

	var moveTime : Float; // how long it takes to move 1 square
	var spinSpeed : Float;
	var spinAmt : Float;
	public var health : Float;

	public var damage : Int;

	public var targetPos : Vector; // for dead-reckoning bullets
	public var targetX : Int;
	public var targetY : Int;

	var isMoving : Bool = false;

	public var playerHit : Creep -> Void;

	public var gameboard : Gameboard;

	public function new( _name : String, _mesh : Mesh )
	{
		super({
			name : _name,
			name_unique : true
			});

		creepName = _name;
		mesh = _mesh;

		// Configure based on name...
		if (_name=='bosscreep')
		{
			spinSpeed = 50.0;
			spinAmt = 0.0;
			moveTime = 0.2;
			damage = 25;
			health = 25;
			//health = 1;
		}
		else // "critter"
		{
			spinSpeed = 30.0;
			spinAmt = 0.0;
			moveTime = 0.5;
			damage = 5;
			health = 3.0;
		}
	}

	public function takeDamage( _damage : Float )
	{
		health -= _damage;
		// trace('creep damaged: HP ${health}');

		if (health <= 0.0)
		{			
			Luxe.events.fire( "creep.killed", this );			
		}
		else
		{
			// just wounded, flash to indicate damage
			// TODO: shader
			// Actuate.tween( mesh.color, 0.2, { r : 1.0, g : 0.0, b : 0.0 })
			// 			.reflect()
			// 			.repeat(3);
		}
	}

	public function setGridTarg( _targX : Int, _targY : Int )
	{
		targetX = _targX;
		targetY = _targY;

		// set the creep targ flag, we're going for this cell
		var creepCell = gameboard.cell( targetX, targetY );
		if (creepCell!=null)
		{
			// might not be a real cell
			creepCell.creeptarg = true;
		}

		isMoving = true;
		targetPos = gameboard.gridPosToWorld( targetX, targetY );
		Actuate.tween( mesh.pos, moveTime, { x : targetPos.x, z : targetPos.z  } )
					.onComplete( function(){
							chooseNextMove();
						});

	}
	
	function chooseNextMove()
	{
		if (destroyed)
		{
			// might have been destroyed already 
			return;
		}

		if (targetY < 0)
		{
			// Already reached home row. damage player
			if (playerHit!=null)
			{
				playerHit( this );
			}
			
			return;
		}

		// DBG: for now move randomly
		// var nextX : Int = targetX + Maths.random_int( -1, 1 );
		// var nextY : Int = targetY + Maths.random_int( -1, 1 );
		// setGridTarg( nextX, nextY );
		var creepCell = gameboard.cell( targetX, targetY );

		// clear the creeptarg flag on this cell, we're moving on
		creepCell.creeptarg = false;

		if (creepCell.gy==0)
		{
			// Already on home row, go in for the kill
			setGridTarg( targetX, targetY-1 );
			return;
		}

		// Go for the closest adjacent cell
		var bestDist = 0.0;		
		var bestCell : BoardCell = null;
		for (cc in gameboard.adjacentCells( creepCell ))
		{
			// is space open and not targeted by another creep?
			if ((!cc.blocked) && (!cc.creeptarg))
			{
				// add some random tiebreak
				var d = cc.homeDist + Maths.random_float( 0.0, 0.5 );
				if ((bestCell==null) || (d < bestDist))
				{
					bestCell = cc;
					bestDist = d;
				}
			}
		}

		// go there
		setGridTarg( bestCell.gx, bestCell.gy );
	}

	override function update(dt:Float) 
	{
		if (mesh==null)
		{
			return;
		}

		// rotate
		mesh.rotation.setFromEuler( new Vector( 0.0, spinAmt, 0.0).radians() );
		spinAmt += spinSpeed * dt;
	}
}
