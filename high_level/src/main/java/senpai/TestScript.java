package senpai;

import senpai.Senpai.ErrorCode;
import senpai.robot.Robot;
import senpai.scripts.Script;
import senpai.scripts.ScriptManager;
import senpai.table.CubeColor;
import senpai.table.Table;
import senpai.threads.comm.ThreadCommProcess;

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
 * Permet de lancer facilement un test
 * @author pf
 *
 */

public class TestScript
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
			senpai.getService(ThreadCommProcess.class).capteursOn = true;
			ScriptManager scripts = senpai.getService(ScriptManager.class);
			Robot robot = senpai.getService(Robot.class);
			Table table = senpai.getService(Table.class);

			Script script = scripts.getAllPossible(true, CubeColor.ORANGE, false).poll();
			script.execute(robot, table);
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
