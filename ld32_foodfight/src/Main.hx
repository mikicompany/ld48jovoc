
import luxe.Input;
import luxe.Mesh;
import luxe.Vector;
import luxe.Parcel;
import luxe.ParcelProgress;
import luxe.Color;
import luxe.Input;
import luxe.Text;

import luxe.collision.ShapeDrawerLuxe;
import luxe.collision.Collision;
import luxe.collision.shapes.*;
import luxe.collision.data.*;

import phoenix.Camera;
import phoenix.geometry.Geometry;
import phoenix.Batcher;

class Bullet {
    public var mesh : Mesh;
    public var vel : Vector;

    public function new ( _mesh : Mesh ) {
        mesh = _mesh;
        vel = new Vector();
    }
}

class Main extends luxe.Game {

    // Player stuff
    var inputLeft : Vector;
    var inputRight : Vector;
    var inputUp : Vector;
    var inputDown : Vector;

    // Game World
    var walls : Array<Shape>;
    var playerShape : Shape;
    var bullets : Array<Bullet>;

    // Geometry
    var playerDir : Vector;
    var strafeDir : Vector;
	var meshWorld : Mesh;	
	var meshPlayer : Mesh;
    var meshBulletSrc : Mesh;

    // Everything is ready
    var gameReadySemaphore : Int = 2;

    // Debug and HUD tools
    var hud_batcher:Batcher;
    var shape_batcher:Batcher;
    var shape_view : Camera;
    var drawer: ShapeDrawerLuxe;
    var desc : Text;

	override function config( config:luxe.AppConfig ) {

        config.render.depth_bits = 24;
        config.render.depth = true;
        config.render.antialiasing=8;

        return config;

    }

    function assets_loaded(_) {

		Luxe.camera.view.set_perspective({
    		far:1000,
            near:0.1,
            fov: 80.0,
            aspect : Luxe.screen.w/Luxe.screen.h
    	});

    	//move up and back a bit
    	Luxe.camera.pos.set_xyz(0,18,7.5);
    	Luxe.camera.rotation.setFromEuler( new Vector( -70.0, 0, 0).radians() );

        // World environment
        var tex = Luxe.loadTexture('assets/ld32_foodfight_env.png');    		
    	meshWorld = new Mesh({ file:'assets/ld32_foodfight_env.obj', 
    					texture:tex, onload:meshloaded });

		// Player
        var texPlayer = Luxe.loadTexture('assets/tmp_player.png');    		
    	meshPlayer = new Mesh({ file:'assets/ld32_foodfight_tmp_player.obj', 
    						texture:texPlayer, onload:meshloaded });

        // Bullet
        var texBullet = Luxe.loadTexture('assets/peas.png');          
        meshBulletSrc = new Mesh({ file:'assets/ld32_foodfight_cube.obj', 
                            texture:texBullet, onload: function (m : Mesh ) {
                                m.geometry.visible = false;
                                meshloaded(null);
                             }});
                            

        // Create the HUD
        create_hud();
    } 

    function create_hud() {

        //For the hud, it has a unique batcher, layer 4 is > the batcher_1, and the default(1)
        hud_batcher = Luxe.renderer.create_batcher({ name:'hud_batcher', layer:4 });

        desc = new Text({
            pos: new Vector(10,10),
            point_size: 18,
            batcher: hud_batcher,
            text: 'Hello there this is ludumdare'
        });

        // Shape batcher
        shape_batcher = Luxe.renderer.create_batcher({ name:'shape_batcher', layer:5 });

        shape_view = new Camera();  
        
        
        //shape_view.zoom = 0.9;

        trace( 'shape_view center is ${shape_view.center}\n');
        trace( 'shape_view pos is ${shape_view.pos}\n');
        trace( 'shape_view zoom is ${shape_view.zoom}\n');
        trace( 'shape_view viewport is ${shape_view.viewport}\n');
        shape_view.pos.set_xy( -shape_view.viewport.w/2.0, -shape_view.viewport.h/2.0 );
        shape_view.zoom = shape_view.viewport.h/25;
        shape_batcher.view = shape_view;
        drawer = new ShapeDrawerLuxe( {
                 batcher: shape_batcher
            });
    }

	override function ready() {

        inputLeft = new Vector();
        inputRight = new Vector();
        inputUp = new Vector();
        inputDown = new Vector();

        playerDir = new Vector( 0.0, 0.0, -1.0 );
        strafeDir = playerDir.clone();
        bullets = new Array<Bullet>();

		var preload = new Parcel();    	
    	preload.add_texture( "assets/ld32_foodfight_env.png");
    	preload.add_texture( "assets/tmp_player.png");

		preload.add_text( "assets/ld32_foodfight_env.obj" );
		preload.add_text( "assets/ld32_foodfight_tmp_player.obj" );

		new ParcelProgress({
            parcel      : preload,
            background  : new Color(0.4,0.4,0.4,0.85),
            oncomplete  : assets_loaded
        });

        preload.load();

        // bind inputs
        Luxe.input.bind_key( 'fire', Key.space );
        Luxe.input.bind_key( 'fire', Key.key_z );
        Luxe.input.bind_gamepad( 'fire', 0 );

        Luxe.input.bind_key( 'moveleft', Key.left );
        Luxe.input.bind_key( 'moveleft', Key.key_a );

        Luxe.input.bind_key( 'moveright', Key.right );
        Luxe.input.bind_key( 'moveright', Key.key_d );

        Luxe.input.bind_key( 'moveup', Key.up );
        Luxe.input.bind_key( 'moveup', Key.key_w );

        Luxe.input.bind_key( 'movedown', Key.down );
        Luxe.input.bind_key( 'movedown', Key.key_s );

        // Create the game world
        walls = [
            // confusing: it's center, w, h
            Polygon.rectangle( -11.0, 0.0, 2.0, 25.0),
            Polygon.rectangle(  11.0, 0.0, 2.0, 25.0),
            Polygon.rectangle(  0.0, -11.0, 25.0, 2.0),
            Polygon.rectangle(  0.0, 11.0, 25.0, 2.0),
            new Circle( 5.0, 5.0, 3.5 )
        ];

        playerShape = new Circle( 0.0, 0.0, 0.4 );

    } // ready

    override function onkeyup( e:KeyEvent ) {

        if(e.keycode == Key.escape) {
            Luxe.shutdown();
        }
	} //onkeyup

	function meshloaded(_) {

        gameReadySemaphore -= 1;

	/*
            //create a second mesh based on the first one
        	mesh2 = new Mesh({
            	geometry : new Geometry({ primitive_type: PrimitiveType.triangles, batcher:Luxe.renderer.batcher }),
            texture : mesh.geometry.texture,
        	});

        	mesh2.geometry.vertices = [].concat(mesh.geometry.vertices);
        	mesh2.transform.pos.set_xy(1,0);
        	*/
    }        

    function cloneMesh( mesh : Mesh ) : Mesh
    {
        var mesh2 = new Mesh({
            geometry : new Geometry({
            batcher : Luxe.renderer.batcher,
            immediate : false,
            primitive_type: PrimitiveType.triangles,
            texture: mesh.geometry.texture
            })
        });

        for(v in mesh.geometry.vertices) {
            mesh2.geometry.add( v.clone() );
        }

        return mesh2;
    }


    function fireUnconventionalWeapon( dt:Float )
    {
        trace('FIRE!');
        var bullet = new Bullet(cloneMesh( meshBulletSrc ));
        
        bullet.mesh.pos.set_xyz( meshPlayer.pos.x, 1.0, meshPlayer.pos.z );
        bullet.vel.copy_from( strafeDir );
        bullets.push( bullet );        
    }

    var next_trace:Float = 0.0;
    override function update(dt:Float) {

        // Is everything initted?
        if (gameReadySemaphore > 0) {
            return;
        }

        // Collisions and movement
        var oldPlayerPos = meshPlayer.pos.clone();

        // update input dir (the 0.1 is for substeps)                       
        var inputDir = new Vector(
            (inputLeft.x + inputRight.x + inputUp.x + inputDown.x) * dt * 0.2,
            (inputLeft.y + inputRight.y + inputUp.y + inputDown.y) * dt * 0.2,
            (inputLeft.z + inputRight.z + inputUp.z + inputDown.z) * dt * 0.2 );

        for ( substep in 0...10)
        {
            // Try the new position for collision
            if ((substep % 2)==0) {
                // Even substep, move x
                playerShape.position.set_xy( playerShape.position.x + inputDir.x,
                                             playerShape.position.y  );
            } else {
                // odd substep, move y
                playerShape.position.set_xy( playerShape.position.x,
                                             playerShape.position.y + inputDir.z );
            }

            var collisions = Collision.shapeWithShapes( playerShape, walls);
            if (collisions.length>0) {

                // Collided, just use the first collider            
                var coll = collisions[0];
                var sep = coll.separation;

                // trace('COLLIDE: ${coll} ${sep} ${collisions.length}');

                var mover_separated_pos = new Vector( coll.shape1.position.x + sep.x*1.01, 
                                                      coll.shape1.position.y + sep.y*1.01 );

                playerShape.position.set_xy( mover_separated_pos.x, mover_separated_pos.y );
            }
            else 
            {
                // no collision here, use the new shape
                meshPlayer.pos.set_xyz( playerShape.position.x, 0.0, playerShape.position.y );
            }
        }

        playerShape.position.set_xy( meshPlayer.pos.x, meshPlayer.pos.z );

        // calc player direction
        var testPlayerDir = new Vector( meshPlayer.pos.x - oldPlayerPos.x,
                                        meshPlayer.pos.y - oldPlayerPos.y,
                                        meshPlayer.pos.z - oldPlayerPos.z );
        if (testPlayerDir.lengthsq > 0.0) {
            playerDir.copy_from( testPlayerDir );
            playerDir.normalize();
        }

        // Shooty shooty
        var firing = false;
        if(Luxe.input.inputdown('fire')) {
            if(Luxe.time > next_trace) {
                trace('fire is held down');
                next_trace = Luxe.time + 0.4;
                firing = true;
            }
        }

        if(Luxe.input.inputpressed('fire')) {
            trace('fire pressed in update');
            fireUnconventionalWeapon( dt );
            firing = true;
        }

        if(Luxe.input.inputreleased('fire')) {
            trace('fire released in update');
        }

        if (!firing) {
            strafeDir.copy_from( playerDir );
        }

        // Update bullets
        var expiredBullets = new Array<Bullet>();
        var bulletShape = new Circle( 0.0, 0.0, 0.25 );
        for ( b in bullets) {
            b.mesh.pos.set_xyz( b.mesh.pos.x + b.vel.x,
                b.mesh.pos.y + b.vel.y,
                b.mesh.pos.z + b.vel.z );

            if (b.mesh.pos.lengthsq > 250.0) {
                expiredBullets.push( b );
            } else {
                // check against walls
                bulletShape.position.set_xy( b.mesh.pos.x, b.mesh.pos.z );
                var collisions = Collision.shapeWithShapes( bulletShape, walls);
                if (collisions.length>0) {
                    expiredBullets.push( b );
                }
            } 
        }

        for ( b in expiredBullets) {
            bullets.remove( b );

            b.mesh.destroy();

        }

        desc.text = 'BULLETS: ${bullets.length}';
    } //update

    override function onrender() {

        // Is everything initted?
        if (gameReadySemaphore > 0) {
            return;
        }

        for (shape in walls) {
            drawer.drawShape(shape);
        }

        drawer.drawShape( playerShape );
    }

    // ===============================================
    //   INPUT HANDLERS
    // ===============================================
    override function oninputup( _input:String, e:InputEvent ) {
        trace( 'named input up : ' + _input );
        if ( _input == 'moveleft') {
                inputLeft.set_xyz( 0.0, 0.0, 0.0 );
            } else if (_input=='moveright') {
                inputRight.set_xyz( 0.0, 0.0, 0.0 );
            } else if (_input=='moveup') {
                inputUp.set_xyz( 0.0, 0.0, 0.0 );
            } else if (_input=='movedown') {
                inputDown.set_xyz( 0.0, 0.0, 0.0 );
            }
    } //oninputup

    override function oninputdown( _input:String, e:InputEvent ) {
        trace( 'named input down : ' + _input );
        var moveSpeed = 15.0;
        if ( _input == 'moveleft') {
                inputLeft.set_xyz( -moveSpeed, 0.0, 0.0 );
            } else if (_input=='moveright') {
                inputRight.set_xyz( moveSpeed, 0.0, 0.0 );
            } else if (_input=='moveup') {
                inputUp.set_xyz( 0.0, 0.0, -moveSpeed );
            } else if (_input=='movedown') {
                inputDown.set_xyz( 0.0, 0.0, moveSpeed );                
            }
    } //oninputdown
} //Main
