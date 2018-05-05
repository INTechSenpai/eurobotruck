package senpai;

import pfg.kraken.exceptions.TimeoutException;
import pfg.kraken.utils.XYO;
import pfg.log.Log;
import senpai.Senpai.ErrorCode;
import senpai.buffer.OutgoingOrderBuffer;
import senpai.comm.CommProtocol;
import senpai.comm.DataTicket;
import senpai.comm.Ticket;
import senpai.exceptions.UnableToMoveException;
import senpai.robot.Robot;
import senpai.robot.RobotColor;
import senpai.scripts.Script;
import senpai.scripts.ScriptManager;
import senpai.scripts.ScriptRecalage;
import senpai.table.Table;
import senpai.threads.comm.ThreadCommProcess;
import senpai.utils.Subject;

/*
 * Copyright (C) 2013-2018 Pierre-François Gimenez
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>
 */

/**
 * Match !
 * @author pf
 *
 */

public class Match
{
	public static void main(String[] args)
	{
		String configfile = "senpai-trajectory.conf";
		

		Senpai senpai = new Senpai();
		ErrorCode error = ErrorCode.NO_ERROR;
		try
		{
			senpai = new Senpai();
			senpai.initialize(configfile, "default", "graphic");
			OutgoingOrderBuffer ll = senpai.getService(OutgoingOrderBuffer.class);
			Robot robot = senpai.getService(Robot.class);
			Table table = senpai.getService(Table.class);
			ScriptManager scripts = senpai.getService(ScriptManager.class);
			Log log = senpai.getService(Log.class);

			RobotColor couleur;
			
			/*
			 * Attente de la couleur
			 */
			DataTicket etat;
			do
			{
				// Demande la couleur toute les 100ms et s'arrête dès qu'elle est connue
				Ticket tc = ll.demandeCouleur();
				etat = tc.attendStatus();
				Thread.sleep(100);
			} while(etat.status != CommProtocol.State.OK);
			couleur = (RobotColor) etat.data;			
			robot.updateColorAndSendPosition(couleur);
			table.updateCote(couleur.symmetry);
			senpai.getService(ThreadCommProcess.class).capteursOn = true;
			
			
			/*
			 * Recalage initial
			 */
			ScriptRecalage rec = scripts.getScriptRecalage(RobotColor.VERT.symmetry);
			rec.execute(robot, table);
			XYO initialCorrection = rec.getCorrection();
			System.out.println(initialCorrection);
			double deltaX = Math.round(initialCorrection.position.getX())/10.;
			double deltaY = Math.round(initialCorrection.position.getY())/10.;
			double orientation = initialCorrection.orientation * 180. / Math.PI;
			log.write("Je suis "+Math.abs(deltaX)+"cm sur la "+(deltaX < 0 ? "droite" : "gauche"), Subject.STATUS);
			log.write("Je suis "+Math.abs(deltaY)+"cm vers l'"+(deltaY < 0 ? "avant" : "arrière"), Subject.STATUS);
			log.write("Je suis orienté vers la "+(orientation < 0 ? "droite" : "gauche")+" de "+Math.abs(orientation)+"°", Subject.STATUS);
			
			Script script = scripts.getScriptDomotique(true);
			boolean restart;
			do {
				try {
					restart = false;
					robot.goTo(script.getPointEntree());
					System.out.println("On est arrivé !");
				}
				catch(UnableToMoveException | TimeoutException e)
				{
					System.out.println("On a eu un problème : "+e);
					restart = true;
				}
			} while(restart);
			
			script.execute(robot, table);
			
			do {
				try {
					restart = false;
					robot.goTo(rec.getPointEntree());
					System.out.println("On est arrivé !");
				}
				catch(UnableToMoveException | TimeoutException e)
				{
					System.out.println("On a eu un problème : "+e);
					restart = true;
				}
			} while(restart);
			
			rec.execute(robot, table);
		}
		catch(Exception e)
		{
			e.printStackTrace();
			error = ErrorCode.EXCEPTION;
			error.setException(e);
		}
		finally
		{
			try
			{
				senpai.destructor(error);
			}
			catch(InterruptedException e)
			{
				e.printStackTrace();
			}
		}
	}
}
