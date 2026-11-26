// Partner environment extensions backed by the IVP simulation and object state.
struct PhysicsPredictionState
{
	CPhysicsObject *object;
	Vector position, velocity, positionError, velocityError;
	QAngle angles;
	AngularImpulse angular;
	bool asleep;
};

struct PhysicsPredictionFrame
{
	int command;
	float deltaTime;
	CUtlVector<PhysicsPredictionState> objects;
};

void CPhysicsEnvironment::SetAlternateGravity( const Vector &gravity )
{
	IVP_U_Point converted;
	ConvertPositionToIVP( gravity, converted );
	m_alternateGravityController->set_standard_gravity( &converted );
}

void CPhysicsEnvironment::GetAlternateGravity( Vector *gravity ) const
{
	ConvertPositionToHL( m_alternateGravityController->grav_vec, *gravity );
}

const IVP_Controller *CPhysicsEnvironment::GetGravityController( bool alternate ) const
{
	return alternate ? m_alternateGravityController : m_pPhysEnv->get_gravity_controller();
}

IVP_Controller *CPhysicsEnvironment::GetGravityController( bool alternate )
{
	return const_cast<IVP_Controller *>( static_cast<const CPhysicsEnvironment *>( this )->GetGravityController( alternate ) );
}

float CPhysicsEnvironment::GetDeltaFrameTime( int maxTicks ) const
{
	// Include the fractional interval to the next PSI, so the first frame
	// can advance even though no previous frame has been simulated.
	return maxTicks > 0 ? MAX( 0.0f, GetNextFrameTime() - GetSimulationTime() ) + GetSimulationTimestep() * (maxTicks - 1) : 0.0f;
}

void CPhysicsEnvironment::ForceObjectsToSleep( IPhysicsObject **objects, int count )
{
	Assert( !m_inSimulation );
	for ( int i = 0; i < count; ++i )
		if ( objects[i] && m_objects.Find( objects[i] ) != m_objects.InvalidIndex() ) objects[i]->Sleep();
	// Publish the sleep events immediately, including the active-object list.
	m_pSleepEvents->UpdateSleepObjects();
}

void CPhysicsEnvironment::ClearPredictionFrames()
{
	m_predictionFrames.PurgeAndDeleteElements();
}

void CPhysicsEnvironment::SetPredicted( bool predicted )
{
	if ( m_predicted != predicted ) ClearPredictionFrames();
	m_predicted = predicted;
}

void CPhysicsEnvironment::CapturePredictionFrame()
{
	PhysicsPredictionFrame *frame = NULL;
	for ( int i = 0; i < m_predictionFrames.Count(); ++i )
		if ( m_predictionFrames[i]->command == m_predictionCommand ) frame = m_predictionFrames[i];
	if ( !frame )
	{
		frame = new PhysicsPredictionFrame;
		m_predictionFrames.AddToTail( frame );
	}
	frame->command = m_predictionCommand;
	frame->deltaTime = m_lastFrameTime;
	frame->objects.RemoveAll();
	for ( int i = 0; i < m_objects.Count(); ++i )
	{
		CPhysicsObject *object = static_cast<CPhysicsObject *>( m_objects[i] );
		if ( object->IsStatic() ) continue;
		PhysicsPredictionState state;
		state.object = object;
		object->GetPosition( &state.position, &state.angles );
		object->GetVelocity( &state.velocity, &state.angular );
		state.positionError = object->GetPredictionErrorPosition();
		state.velocityError = object->GetPredictionErrorVelocity();
		state.asleep = object->IsAsleep();
		frame->objects.AddToTail( state );
	}
}

void CPhysicsEnvironment::ForgetPredictedObject( IPhysicsObject *object )
{
	// Do this before deletion or an environment transfer: addresses can be reused.
	for ( int i = 0; i < m_predictionFrames.Count(); ++i )
		for ( int j = m_predictionFrames[i]->objects.Count() - 1; j >= 0; --j )
			if ( m_predictionFrames[i]->objects[j].object == object ) m_predictionFrames[i]->objects.FastRemove( j );
}

void CPhysicsEnvironment::DoneReferencingPreviousCommands( int command )
{
	for ( int i = m_predictionFrames.Count() - 1; i >= 0; --i )
		if ( m_predictionFrames[i]->command < command )
		{
			delete m_predictionFrames[i];
			m_predictionFrames.FastRemove( i );
		}
}

void CPhysicsEnvironment::RestorePredictedSimulation()
{
	if ( !m_predicted ) return;
	for ( int i = 0; i < m_predictionFrames.Count(); ++i )
	{
		const PhysicsPredictionFrame &frame = *m_predictionFrames[i];
		if ( frame.command != m_predictionCommand ) continue;
		for ( int j = 0; j < frame.objects.Count(); ++j )
		{
			const PhysicsPredictionState &state = frame.objects[j];
			CPhysicsObject *object = state.object;
			Vector position = state.position + object->GetPredictionErrorPosition() - state.positionError;
			Vector velocity = state.velocity + object->GetPredictionErrorVelocity() - state.velocityError;
			object->SetPosition( position, state.angles, true );
			object->SetVelocityInstantaneous( &velocity, &state.angular );
			if ( state.asleep ) object->Sleep(); else object->Wake();
		}
		m_lastFrameTime = frame.deltaTime;
		m_pSleepEvents->UpdateSleepObjects();
		return;
	}
}

void CPhysicsEnvironment::DestroyCollideOnDeadObjectFlush( CPhysCollide *collide )
{
	if ( collide && m_deadCollides.Find( collide ) == m_deadCollides.InvalidIndex() ) m_deadCollides.AddToTail( collide );
	if ( !m_inSimulation && !m_queueDeleteObject ) ClearDeadObjects();
}
